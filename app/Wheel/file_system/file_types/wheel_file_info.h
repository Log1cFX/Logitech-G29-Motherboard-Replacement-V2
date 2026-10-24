/*
 * wheel_file_info.h
 *
 *  Created on: Oct 24, 2026
 *      Author: raffi
 *
 *  Content of a WHEEL_FILE_INFO file (see wheel_file.h): what the firmware tells
 *  of itself, the answer to WHEEL_COMMAND_INFO (wheel_file_command.h)
 */

#ifndef FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_INFO_H_
#define FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_file_error.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// Bits of build
#define WHEEL_FILE_INFO_BUILD_DEBUG 0x01u // DEBUG: the logs go to semihosting
// WHEEL_SKIP_CALIBRATION: a board on the bench (wheel_def.h)
#define WHEEL_FILE_INFO_BUILD_SKIP_CALIBRATION 0x02u

typedef struct {
  uint32_t uid[3]; // unique ID of the STM32 (UID_BASE), its words in order
  uint32_t uptime_ms; // HAL_GetTick() when the file was built
  uint8_t build; // WHEEL_FILE_INFO_BUILD_* bits
  // WHEEL_LOG_LEVEL of the build: the highest log_level of the settings
  uint8_t log_level_max;
  uint16_t error_codes; // WHEEL_ERR_COUNT: the entries of error_counts
  uint32_t dropped_errors; // soft errors not recorded: the ring was full
  // Texts of WHEEL_LOG() not sent: the file link was full, or the call came
  // from an interrupt
  uint32_t dropped_logs;
  // Every soft error since the start, by code (wheel_error_t), merged and
  // dropped ones included
  uint32_t error_counts[WHEEL_ERR_COUNT];
} wheel_file_info_t;

// The host reads the fields at these places. A new error code makes the file
// longer: the host takes error_codes entries
static_assert(offsetof(wheel_file_info_t, uptime_ms) == 12, "a field moved");
static_assert(offsetof(wheel_file_info_t, build) == 16, "a field moved");
static_assert(offsetof(wheel_file_info_t, dropped_errors) == 20, "a field moved");
static_assert(offsetof(wheel_file_info_t, error_counts) == 28, "a field moved");
static_assert(sizeof(wheel_file_info_t) == 100,
    "not 100 bytes: WHEEL_ERR_COUNT changed, the host must know");

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_INFO_H_ */
