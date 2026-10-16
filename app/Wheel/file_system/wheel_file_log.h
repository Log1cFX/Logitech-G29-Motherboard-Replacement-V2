/*
 * wheel_file_log.h
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Content of a WHEEL_FILE_LOG file (see wheel_file.h): one text of
 *  WHEEL_LOG()
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_LOG_H_
#define FILE_SYSTEM_WHEEL_FILE_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Version of wheel_file_log_t, for the header of the file. Changes with the
// layout of the struct
#define WHEEL_FILE_LOG_VERSION 1

// Longest text: a longer one is cut
#define WHEEL_FILE_LOG_MAX_TEXT 120

typedef struct {
  uint32_t time_ms; // HAL_GetTick() when it was written: ms since the start
  // header.size - 4 characters, without a terminating zero
  char text[];
} wheel_file_log_t;

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_LOG_H_ */
