#!/usr/bin/env python3
# Patches a firmware image (.bin or .elf) to hold the right checksum
# Checksum is a 32bit field at offset 0x1C, whereas
# firmware size is stored at 0x20 (little endian, words)

import sys, struct

data = bytearray(open(sys.argv[1], "rb").read())

if data[:4] == b"\x7fELF":
	# Rebuild the flash image from the loadable segments (like objcopy -O binary)
	phoff, = struct.unpack_from("<I", data, 0x1C)
	phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
	segs = []
	for i in range(phnum):
		ptype, off, vaddr, paddr, filesz = struct.unpack_from("<5I", data, phoff + i * phentsize)
		if ptype == 1 and filesz:
			segs.append((paddr, off, filesz))
	segs.sort()
	base, hdr = segs[0][0], segs[0][1]
	fw = bytearray(segs[-1][0] + segs[-1][2] - base)
	for paddr, off, filesz in segs:
		fw[paddr - base:paddr - base + filesz] = data[off:off + filesz]
else:
	fw, hdr = bytearray(data), 0

# Ensure the firmware is word size aligned
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

# Overwrite firmware file
struct.pack_into("<II", data, hdr + 0x1C, xorv, len(fw) // 4)
open(sys.argv[1], "wb").write(data)
