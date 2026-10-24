/*
 * wheel_command.c
 *
 *  Created on: Oct 24, 2026
 *      Author: raffi
 *
 *  The commands of the PC, see wheel_command.h.
 *
 *  STEPS
 *  A command is carried out in two steps. Each one is done once, even when the
 *  link is full and a later call has to finish the command:
 *
 *    1. its action: a save, a revert, or the files a read or INFO asks for.
 *       These go all at once, with room left for the status, or none of them
 *       goes (WHEEL_COMMAND_LINK_FULL). The result goes into the status
 *    2. its status, which waits for room. Then the command leaves the buffer
 *       of the link
 *
 *  A reboot does its reset once its status has left the link.
 *
 *  The files of a command are copies: usb_msg_send() takes the content when it
 *  is called
 */

#include "wheel_command.h"
#include "wheel_settings.h"
#include "wheel_log.h"
#include "usb_msg.h"
#include "bootloader.h"
#include "main.h"

#include <string.h>

// What the status takes in the buffer of the link: the files of an answer
// always leave it this room
#define STATUS_ROOM USB_MSG_FILE_ROOM(sizeof(wheel_file_command_status_t))

// Main only
static struct {
  Wheel_HandleTypeDef *wheel;
  void (*settings_changed)(void);

  // The command being handled: the oldest file of the PC. It stays in the
  // buffer of usb_msg until its status is stored
  bool acted; // step 1 is done: status holds its result
  wheel_file_command_status_t status;

  // A reset waits for the status of a reboot: the code of the reboot, 0 when
  // none waits. Then the crc field of the status, for usb_msg_pending()
  uint8_t reboot;
  uint32_t reboot_crc;
} cmd;

// The motor would keep its last force while the firmware stalls or resets. The
// control loop sets the force again at its next step
static void stop_motor(void) {
  Actuator_HandleTypeDef *actuator = cmd.wheel->hActuator;
  actuator->Apply_Force(actuator, 0);
}

/* STEP 1 */

// The erases of the save stall the control loop
static uint8_t save(void) {
  stop_motor();
  return wheel_settings_save() ? WHEEL_COMMAND_OK : WHEEL_COMMAND_FLASH_ERROR;
}

// The ffb library keeps copies of some settings: it is told of the change
static uint8_t revert(void) {
  if (!wheel_settings_revert()) {
    return WHEEL_COMMAND_FLASH_ERROR;
  }
  cmd.settings_changed();
  return WHEEL_COMMAND_OK;
}

// The settings files of the image: every group when arg is 0, else the group of
// arg only (checked by the link). All of them, or none when they do not fit
// with the status
static uint8_t send_settings(uint8_t arg) {
  uint16_t room = STATUS_ROOM;
  for (uint8_t type = WHEEL_FILE_SETTINGS_FIRST; type <= WHEEL_FILE_SETTINGS_LAST;
      type++) {
    const wheel_file_t *file = wheel_settings_file(type);
    if (file != NULL && (arg == 0u || arg == type)) {
      room += USB_MSG_FILE_ROOM(file->header.size);
    }
  }
  if (room > usb_msg_room()) {
    return WHEEL_COMMAND_LINK_FULL;
  }
  // The room is there: no send fails
  for (uint8_t type = WHEEL_FILE_SETTINGS_FIRST; type <= WHEEL_FILE_SETTINGS_LAST;
      type++) {
    const wheel_file_t *file = wheel_settings_file(type);
    if (file != NULL && (arg == 0u || arg == type)) {
      (void)usb_msg_send(file->header.type, file->content, file->header.size);
    }
  }
  return WHEEL_COMMAND_OK;
}

// The info file, or nothing when it does not fit with the status
static uint8_t send_info(void) {
  if (USB_MSG_FILE_ROOM(sizeof(wheel_file_info_t)) + STATUS_ROOM > usb_msg_room()) {
    return WHEEL_COMMAND_LINK_FULL;
  }
  wheel_file_info_t info = {0};
  info.uid[0] = HAL_GetUIDw0();
  info.uid[1] = HAL_GetUIDw1();
  info.uid[2] = HAL_GetUIDw2();
  info.uptime_ms = HAL_GetTick();
#ifdef DEBUG
  info.build |= WHEEL_FILE_INFO_BUILD_DEBUG;
#endif
#if WHEEL_SKIP_CALIBRATION
  info.build |= WHEEL_FILE_INFO_BUILD_SKIP_CALIBRATION;
#endif
  info.log_level_max = WHEEL_LOG_LEVEL;
  info.error_codes = WHEEL_ERR_COUNT;
  wheel_log_counts(info.error_counts, &info.dropped_errors, &info.dropped_logs);
  // The room is there: the send does not fail
  (void)usb_msg_send(WHEEL_FILE_INFO, &info, sizeof(info));
  return WHEEL_COMMAND_OK;
}

// The action of the command, which gives its result. A read and INFO send their
// files here. The reboots have nothing to do before their status
static uint8_t act(const wheel_file_command_t *command) {
  switch (command->code) {
  case WHEEL_COMMAND_READ_SETTINGS:
    return send_settings(command->arg);
  case WHEEL_COMMAND_SAVE_SETTINGS:
    return save();
  case WHEEL_COMMAND_REVERT_SETTINGS:
    return revert();
  case WHEEL_COMMAND_INFO:
    return send_info();
  default:
    return WHEEL_COMMAND_OK;
  }
}

/* STEP 2 */

// The status, last file of the command. False when the link is full. A reboot
// then waits for it to leave: usb_msg builds the same bytes as the copy made
// here, so the same crc field
static bool send_status(const wheel_file_command_t *command) {
  if (usb_msg_send(WHEEL_FILE_COMMAND_STATUS, &cmd.status, sizeof(cmd.status)) != USB_MSG_OK) {
    return false;
  }
  if (command->code == WHEEL_COMMAND_REBOOT || command->code
      == WHEEL_COMMAND_REBOOT_DFU) {
    // Words: a file starts on a multiple of 4 bytes
    uint32_t copy[WHEEL_FILE_TOTAL_SIZE(sizeof(cmd.status)) / 4u];
    wheel_file_t *file = (wheel_file_t*)copy;
    wheel_file_make(file, WHEEL_FILE_COMMAND_STATUS, &cmd.status,
                    sizeof(cmd.status));
    cmd.reboot_crc = file->header.crc;
    cmd.reboot = command->code;
  }
  return true;
}

/* COMMANDS */

// The two steps of a command, each done once. True once its status is stored:
// the command can leave the buffer of the link
static bool handle(const wheel_file_command_t *command) {
  if (!cmd.acted) {
    cmd.status.code = command->code;
    cmd.status.id = command->id;
    cmd.status.result = act(command);
    cmd.acted = true;
  }
  if (!send_status(command)) {
    return false;
  }
  // The next command starts from step 1
  cmd.acted = false;
  return true;
}

// The reset of a reboot, once its status has left the link, delivered or
// refused. Or once the link is closed: nobody would read the status
static void reboot_when_answered(void) {
  if (usb_msg_connected() && usb_msg_pending(cmd.reboot_crc)) {
    return;
  }
  stop_motor();
  if (cmd.reboot == WHEEL_COMMAND_REBOOT_DFU) {
    bootloader_reboot_into_dfu();
  }
  NVIC_SystemReset();
}

/* INTERFACE (wheel_command.h) */

// Nothing is handled yet, no reset waits
void wheel_command_init(Wheel_HandleTypeDef *wheel, void (*settings_changed)(void)) {
  memset(&cmd, 0, sizeof(cmd));
  cmd.wheel = wheel;
  cmd.settings_changed = settings_changed;
}

// A file leaves the buffer of the link once it is handled. Until then each call
// takes it again
void wheel_command_task(void) {
  // After a reboot nothing else is handled: the reset would lose it
  if (cmd.reboot != 0u) {
    reboot_when_answered();
    return;
  }

  const wheel_file_t *file = usb_msg_peek();
  if (file == NULL) {
    return;
  }
  uint8_t type = file->header.type;
  if (wheel_settings_file(type) != NULL) {
    // Checked by the link, its ranges included: straight into the image
    wheel_settings_receive(file);
    cmd.settings_changed();
    usb_msg_pop();
  } else if (type == WHEEL_FILE_COMMAND) {
    // The content of a file of the link lies on a multiple of 4 bytes. A
    // command whose files do not fit is answered WHEEL_COMMAND_LINK_FULL: only
    // its status can make it wait
    if (handle((const wheel_file_command_t*)file->content)) {
      usb_msg_pop();
    }
  } else {
    // A file of a type only the wheel sends, or keeps for itself
    usb_msg_pop();
  }
}
