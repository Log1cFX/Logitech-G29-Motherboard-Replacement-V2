/*
 * wheel_file_types.h
 *
 *  Created on: Oct 20, 2026
 *      Author: raffi
 *
 *  Every type of file (wheel_file.h) and the struct of its content. Like the
 *  format, depends on nothing of the firmware: the host builds the same structs
 */

#ifndef FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_TYPES_H_
#define FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_file_command.h"
#include "wheel_file_dump.h"
#include "wheel_file_error.h"
#include "wheel_file_info.h"
#include "wheel_file_log.h"
#include "wheel_file_settings.h"

// What a file contains: its content is cast to the struct of its type.
// Stored in one byte. 0x00 and 0xFF are never a type: cleared and erased memory
typedef enum {
  WHEEL_FILE_DUMP = 2, // wheel_file_dump_t
  WHEEL_FILE_ERROR = 3, // wheel_file_error_t
  WHEEL_FILE_LOG = 4, // wheel_file_log_t
  // A command of the PC and its answers: see wheel_command.h
  WHEEL_FILE_COMMAND = 5,  // wheel_file_command_t, from the PC
  WHEEL_FILE_COMMAND_STATUS = 6,  // wheel_file_command_status_t, to the PC
  WHEEL_FILE_INFO = 7,  // wheel_file_info_t, to the PC
  // Settings, one file per group: see wheel_settings.h
  WHEEL_FILE_SETTINGS_FFB = 0x10,  // wheel_file_settings_ffb_t
  WHEEL_FILE_SETTINGS_STEERING = 0x11,  // wheel_file_settings_steering_t
  WHEEL_FILE_SETTINGS_CALIBRATION = 0x12,  // wheel_file_settings_calibration_t
  WHEEL_FILE_SETTINGS_SHIFTER = 0x13,  // wheel_file_settings_shifter_t
  WHEEL_FILE_SETTINGS_PEDALS = 0x14,  // wheel_file_settings_pedals_t
  WHEEL_FILE_SETTINGS_BUTTONS = 0x15,  // wheel_file_settings_buttons_t
  WHEEL_FILE_SETTINGS_SYSTEM = 0x16,  // wheel_file_settings_system_t
  WHEEL_FILE_SETTINGS_FORCE = 0x17  // wheel_file_settings_force_t
} wheel_file_type_t;

// The first and the last settings type: the settings types follow each other
#define WHEEL_FILE_SETTINGS_FIRST WHEEL_FILE_SETTINGS_FFB
#define WHEEL_FILE_SETTINGS_LAST WHEEL_FILE_SETTINGS_FORCE

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_TYPES_H_ */
