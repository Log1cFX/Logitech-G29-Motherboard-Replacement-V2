/*
 * wheel_file_command.h
 *
 *  Created on: Oct 24, 2026
 *      Author: raffi
 *
 *  Content of a WHEEL_FILE_COMMAND file, sent by the PC, and of the
 *  WHEEL_FILE_COMMAND_STATUS file that answers it (see wheel_file.h). What the
 *  wheel does with them: wheel_command.h
 */

#ifndef FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_COMMAND_H_
#define FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

// What a command asks for. The values are part of the file: a code keeps its
// value, a new one goes at the end of the list. 0 is never a code
typedef enum {
  // The settings files of the RAM image: the one of arg, or every group
  WHEEL_COMMAND_READ_SETTINGS = 1,
  WHEEL_COMMAND_SAVE_SETTINGS = 2, // the RAM image to the flash
  WHEEL_COMMAND_REVERT_SETTINGS = 3, // the settings page back over the image
  WHEEL_COMMAND_INFO = 4, // a WHEEL_FILE_INFO file (wheel_file_info.h)
  WHEEL_COMMAND_REBOOT = 5,
  WHEEL_COMMAND_REBOOT_DFU = 6 // the bootloader then stays in DFU mode
} wheel_command_code_t;

// How a command went, in its status. The values are part of the file
typedef enum {
  WHEEL_COMMAND_OK = 0,
  // SAVE_SETTINGS: the backup page could not be written, the settings page is
  // as before. REVERT_SETTINGS: the settings page holds no valid settings
  WHEEL_COMMAND_FLASH_ERROR = 1,
  // READ_SETTINGS, INFO: the buffer of the file link had no room for all the
  // files of the answer. None was sent: the PC asks again later
  WHEEL_COMMAND_LINK_FULL = 2
} wheel_command_result_t;

typedef struct {
  uint8_t code; // wheel_command_code_t
  // WHEEL_COMMAND_READ_SETTINGS: a WHEEL_FILE_SETTINGS_* type, 0 for every
  // group. 0 for the other codes
  uint8_t arg;
  uint16_t id; // picked by the PC, sent back in the status
} wheel_file_command_t;
static_assert(sizeof(wheel_file_command_t) == 4,
    "wheel_file_command_t is not 4 bytes");

// The last file of a command: it follows the files the command asked for
typedef struct {
  uint8_t code; // of the command
  uint8_t result; // wheel_command_result_t
  uint16_t id; // of the command
} wheel_file_command_status_t;
static_assert(sizeof(wheel_file_command_status_t) == 4,
    "wheel_file_command_status_t is not 4 bytes");

/* CHECK */
// In wheel_file_command.c. True when the wheel knows the code, and the argument
// is one the code takes
bool wheel_file_command_valid(const wheel_file_command_t *command);

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_COMMAND_H_ */
