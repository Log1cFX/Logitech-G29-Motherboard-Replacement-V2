#!/usr/bin/env python3
# Patches the firmware ELF for the bootloader, then writes the .bin from it:
#   python checksum.py firmware.elf [firmware.bin]
#
# 1. The CRC of each settings file of the .settings section (the settings
#    page, see wheel_settings.h), computed as the CRC unit of the STM32 does
#    (CRC in wheel_file.h)
# 2. The checksum of the app image, from _sapp to _eapp: a 32bit field at
#    offset 0x1C, whereas the firmware size is stored at 0x20 (little endian,
#    words). The settings page is out of it: it changes at run time
# 3. The .bin: every loadable segment at its load address, from _sapp to the
#    end of the settings page, the gaps filled with 0xFF (erased flash)

import sys, struct

ELF_MAGIC = b"\x7fELF"
PT_LOAD = 1
SHT_SYMTAB = 2
SETTINGS_SECTION = b".settings"
FILE_HEADER_SIZE = 8  # wheel_file_header_t
CRC_POLY = 0x04C11DB7
MAX_BIN_SIZE = 128 * 1024  # the whole flash

def sections(data):
	"""Name, file offset, size and type of each section"""
	shoff, = struct.unpack_from("<I", data, 0x20)
	shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)
	headers = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
	names_offset = headers[shstrndx][4]
	result = []
	for h in headers:
		name = data[names_offset + h[0]:data.index(b"\0", names_offset + h[0])]
		result.append({"name": name, "type": h[1], "offset": h[4], "size": h[5],
		               "link": h[6], "entsize": h[9]})
	return result

def symbol(data, all_sections, wanted):
	"""Value of a symbol of the symbol table"""
	for s in all_sections:
		if s["type"] != SHT_SYMTAB:
			continue
		names = all_sections[s["link"]]["offset"]
		for i in range(s["size"] // s["entsize"]):
			name_offset, value = struct.unpack_from("<II", data, s["offset"] + i * s["entsize"])
			name = data[names + name_offset:data.index(b"\0", names + name_offset)]
			if name == wanted:
				return value
	sys.exit("ERROR: no symbol %s, file not patched" % wanted.decode())

def load_segments(data):
	"""Load address, file offset and size of each loadable segment, in order"""
	phoff, = struct.unpack_from("<I", data, 0x1C)
	phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
	segs = []
	for i in range(phnum):
		ptype, off, vaddr, paddr, filesz = struct.unpack_from("<5I", data, phoff + i * phentsize)
		if ptype == PT_LOAD and filesz:
			segs.append((paddr, off, filesz))
	return sorted(segs)

def crc_stm32(words):
	"""CRC unit of the STM32: each word from its most significant bit"""
	crc = 0xFFFFFFFF
	for word in words:
		crc ^= word
		for _ in range(32):
			crc = ((crc << 1) ^ CRC_POLY) if crc & 0x80000000 else (crc << 1)
			crc &= 0xFFFFFFFF
	return crc

def patch_settings(data, settings):
	"""Fills in the crc of each file of the settings section"""
	offset = settings["offset"]
	end = offset + settings["size"]
	while offset < end:
		ftype, version, size = struct.unpack_from("<BBH", data, offset)
		padded = (size + 3) & ~3
		if offset + FILE_HEADER_SIZE + padded > end:
			sys.exit("ERROR: settings file 0x%02X runs past its section" % ftype)
		# The first 4 bytes of the header, then the content with its padding
		covered = data[offset:offset + 4] + data[offset + FILE_HEADER_SIZE:offset + FILE_HEADER_SIZE + padded]
		crc = crc_stm32(w for w, in struct.iter_unpack("<I", covered))
		struct.pack_into("<I", data, offset + 4, crc)
		print("Settings file 0x%02X, %d bytes, crc 0x%08X" % (ftype, size, crc))
		offset += FILE_HEADER_SIZE + padded

def image(data, segs, start, end):
	"""The flash from start to end, as the segments fill it"""
	fw = bytearray(b"\xFF" * (end - start))
	for paddr, off, filesz in segs:
		lo, hi = max(paddr, start), min(paddr + filesz, end)
		if lo < hi:
			fw[lo - start:hi - start] = data[off + lo - paddr:off + hi - paddr]
	return fw

data = bytearray(open(sys.argv[1], "rb").read())
if data[:4] != ELF_MAGIC:
	sys.exit("ERROR: %s is not an ELF file, file not patched" % sys.argv[1])

all_sections = sections(data)
settings = [s for s in all_sections if s["name"] == SETTINGS_SECTION]
if len(settings) != 1:
	sys.exit("ERROR: no .settings section, file not patched")
patch_settings(data, settings[0])

# The app image, without the settings page
sapp = symbol(data, all_sections, b"_sapp")
eapp = symbol(data, all_sections, b"_eapp")
segs = load_segments(data)
fw = image(data, segs, sapp, eapp)
print("Firmware size", len(fw))
if len(fw) % 4 != 0:
	sys.exit("ERROR: firmware is not word size aligned, file not patched")

# Patch 0x1C with zero, 0x20 with the FW size
struct.pack_into("<II", fw, 0x1C, 0, len(fw) // 4)

# Calculate the checksum over the whole image
xorv = 0xB4DC0FEE
for word, in struct.iter_unpack("<I", fw):
	xorv ^= word
print("Checksum 0x%08X" % xorv)

# Patch the ELF, in the segment that holds _sapp
first = [s for s in segs if s[0] <= sapp < s[0] + s[2]]
if not first:
	sys.exit("ERROR: no segment at _sapp, file not patched")
paddr, off, filesz = first[0]
struct.pack_into("<II", data, off + sapp - paddr + 0x1C, xorv, len(fw) // 4)
open(sys.argv[1], "wb").write(data)

# The .bin, from the patched ELF: the app, the gap, the settings page
if len(sys.argv) > 2:
	end = max(p + size for p, o, size in segs)
	if min(p for p, o, size in segs) < sapp or end - sapp > MAX_BIN_SIZE:
		sys.exit("ERROR: a segment is loaded out of the flash, no .bin written")
	open(sys.argv[2], "wb").write(image(data, segs, sapp, end))
	print("Binary size", end - sapp)
