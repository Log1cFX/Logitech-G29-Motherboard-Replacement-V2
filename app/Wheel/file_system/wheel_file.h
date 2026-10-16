/*
 * wheel_file.h
 *
 *  Created on: Oct 11, 2026
 *      Author: raffi
 *
 *  Format of a file: a header, then its content. The same bytes are stored in
 *  flash (wheel_file_pages.h) and exchanged with the host.
 *  Depends on nothing of the firmware: the host builds the same structs
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_H_
#define FILE_SYSTEM_WHEEL_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>

// What a file contains: its content is cast to the struct of its type.
// Stored in one byte. 0x00 and 0xFF are never a type: cleared and erased memory
typedef enum {
  WHEEL_FILE_SETTINGS = 1, // wheel_file_settings_t
  WHEEL_FILE_DUMP = 2, // wheel_file_dump_t
  WHEEL_FILE_ERROR = 3, // wheel_file_error_t
  WHEEL_FILE_LOG = 4 // wheel_file_log_t
} wheel_file_type_t;

/*
 * CRC
 * Covers the first 4 bytes of the header, then the content padded with zeros
 * to a multiple of 4 bytes, both taken as little-endian 32-bit words.
 * Computed like the CRC unit of the STM32: polynomial 0x04C11DB7, initial
 * value 0xFFFFFFFF, each word from its most significant bit, no final XOR.
 */
typedef struct {
  uint8_t type; // wheel_file_type_t
  uint8_t version; // of the layout of the content, one numbering per type
  uint16_t size; // of the content in bytes, without its padding
  uint32_t crc; // see CRC above
} wheel_file_header_t;

// The content follows the header: it has to start on a multiple of 4 bytes
static_assert(sizeof(wheel_file_header_t) == 8, "wheel_file_header_t is not 8 bytes");

// A file where it lies: a flash page, the buffer of the USB module. The
// address of its first byte, a multiple of 4, is cast to a wheel_file_t*.
// content holds header.size bytes, then zeros up to a multiple of 4 bytes
typedef struct {
  wheel_file_header_t header;
  uint8_t content[];
} wheel_file_t;

// Size of a content with its padding, then of the whole file
#define WHEEL_FILE_PADDED_SIZE(size) (((size) + 3u) & ~3u)
#define WHEEL_FILE_TOTAL_SIZE(size) \
  (sizeof(wheel_file_header_t) + WHEEL_FILE_PADDED_SIZE(size))

/* HELPERS */
// In wheel_file.c. Main only: they share the CRC unit of the MCU
// What wheel_file_check() found
typedef enum {
  WHEEL_FILE_OK,
  WHEEL_FILE_BAD_CRC,
  WHEEL_FILE_UNKNOWN_TYPE,
  WHEEL_FILE_BAD_VERSION,
  WHEEL_FILE_BAD_SIZE // not a size its type allows
} wheel_file_status_t;

// CRC of a file, computed as described above. Reads header.size bytes of
// content: the caller makes sure they are there (a flash page, a buffer)
uint32_t wheel_file_crc(const wheel_file_t *file);

// Builds a file at file, which needs WHEEL_FILE_TOTAL_SIZE(size) bytes: the
// header with its CRC, a copy of the content, the padding
void wheel_file_make(wheel_file_t *file, uint8_t type, uint8_t version,
                     const void *content, uint16_t size);

// The one place that knows what a valid file is: its CRC, then the rules of
// its type. A content can be cast to its struct once this returns WHEEL_FILE_OK
wheel_file_status_t wheel_file_check(const wheel_file_t *file);

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_H_ */
