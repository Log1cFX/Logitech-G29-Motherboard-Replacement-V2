/*
 * wheel_file_settings.h
 *
 *  Created on: Oct 11, 2026
 *      Author: raffi
 *
 *  Content of a WHEEL_FILE_SETTINGS file (see wheel_file.h)
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_SETTINGS_H_
#define FILE_SYSTEM_WHEEL_FILE_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Version of wheel_file_settings_t, for the header of the file. Changes with
// the layout of the struct
#define WHEEL_FILE_SETTINGS_VERSION 1

typedef struct {
  // Incremented at each save. When both settings pages hold a valid file, the
  // one with the highest count is the current one (see wheel_file_pages.h)
  uint32_t save_count;
} wheel_file_settings_t;

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_SETTINGS_H_ */
