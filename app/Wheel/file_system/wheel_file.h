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
  WHEEL_FILE_DUMP = 2
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
  uint32_t crc;
} wheel_file_header_t;

// The content follows the header: it has to start on a multiple of 4 bytes
static_assert(sizeof(wheel_file_header_t) == 8, "wheel_file_header_t is not 8 bytes");

// A file in RAM. content is not stored: it points to the bytes that follow the
// header (in flash, in a message of the host) or to the struct of the caller
typedef struct {
  wheel_file_header_t header;
  const void *content;
} wheel_file_t;

// Size of a content with its padding, then of the whole file
#define WHEEL_FILE_PADDED_SIZE(size) (((size) + 3u) & ~3u)
#define WHEEL_FILE_TOTAL_SIZE(size) \
  (sizeof(wheel_file_header_t) + WHEEL_FILE_PADDED_SIZE(size))

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_H_ */
