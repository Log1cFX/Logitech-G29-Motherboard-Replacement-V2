/*
 * wheel_file.c
 *
 *  Created on: Oct 13, 2026
 *      Author: raffi
 *
 *  Helpers of the file format, see wheel_file.h.
 *  The CRC comes from the CRC unit of the MCU, which the CubeMX code of main.c
 *  starts. The unit holds one computation at a time: main only
 */

#include "wheel_file.h"
#include "wheel_file_settings.h"
#include "main.h"
#include <stddef.h>
#include <string.h>

// Gives size bytes to the CRC unit as little-endian words, the last one padded
// with zeros
static void crc_add(const void *data, uint16_t size) {
  const uint8_t *bytes = data;
  while (size > 0) {
    // 4 bytes per word. The last word may get fewer: its other bytes stay 0
    uint16_t count = (size < 4) ? size : 4;
    uint32_t word = 0;
    memcpy(&word, bytes, count);
    // Writing DR adds the word to the CRC being computed
    CRC->DR = word;
    bytes += count;
    size -= count;
  }
}

// CRC of a file, see CRC in wheel_file.h. The unit starts again from its
// initial value, takes the header up to its crc field, then the content:
// reading DR gives the result.
// header.size bytes of content are read: the caller makes sure they are there
uint32_t wheel_file_crc(const wheel_file_t *file) {
  CRC->CR = CRC_CR_RESET;
  crc_add(&file->header, offsetof(wheel_file_header_t, crc));
  crc_add(file->content, file->header.size);
  return CRC->DR;
}

// Builds a file at file: the header, a copy of the content, then zeros up to a
// multiple of 4 bytes. The CRC comes last: it covers what was just written
void wheel_file_make(wheel_file_t *file, uint8_t type, uint8_t version,
                     const void *content, uint16_t size) {
  file->header.type = type;
  file->header.version = version;
  file->header.size = size;
  if (size > 0) {
    memcpy(file->content, content, size);
  }
  memset(&file->content[size], 0, WHEEL_FILE_PADDED_SIZE(size) - size);
  file->header.crc = wheel_file_crc(file);
}

// What is wrong with a file, WHEEL_FILE_OK when nothing is
wheel_file_status_t wheel_file_check(const wheel_file_t *file) {
  const wheel_file_header_t *header = &file->header;
  // The CRC first: the other fields mean nothing when it is wrong
  if (wheel_file_crc(file) != header->crc) {
    return WHEEL_FILE_BAD_CRC;
  }

  // Then the rules of its type
  switch (header->type) {
  case WHEEL_FILE_SETTINGS:
    if (header->version != WHEEL_FILE_SETTINGS_VERSION) {
      return WHEEL_FILE_BAD_VERSION;
    }
    if (header->size != sizeof(wheel_file_settings_t)) {
      return WHEEL_FILE_BAD_SIZE;
    }
    return WHEEL_FILE_OK;
  case WHEEL_FILE_DUMP:
    // Its content is not defined yet: nothing more to check
    return WHEEL_FILE_OK;
  default:
    return WHEEL_FILE_UNKNOWN_TYPE;
  }
}
