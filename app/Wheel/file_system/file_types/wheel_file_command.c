/*
 * wheel_file_command.c
 *
 *  Created on: Oct 24, 2026
 *      Author: raffi
 *
 *  What a valid command is, see wheel_file_command.h. The command module relies
 *  on it: it never checks a code or an argument again
 */

#include "wheel_file_types.h"

// The argument of each code. Any other code is unknown: false
bool wheel_file_command_valid(const wheel_file_command_t *command) {
  switch (command->code) {
  case WHEEL_COMMAND_READ_SETTINGS:
    // Every group, or one settings type
    return command->arg == 0u
        || (command->arg >= WHEEL_FILE_SETTINGS_FIRST && command->arg
            <= WHEEL_FILE_SETTINGS_LAST);
  case WHEEL_COMMAND_SAVE_SETTINGS:
  case WHEEL_COMMAND_REVERT_SETTINGS:
  case WHEEL_COMMAND_INFO:
  case WHEEL_COMMAND_REBOOT:
  case WHEEL_COMMAND_REBOOT_DFU:
    // No argument
    return command->arg == 0u;
  default:
    return false;
  }
}
