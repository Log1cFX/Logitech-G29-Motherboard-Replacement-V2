/*
 * wheel_command.h
 *
 *  Created on: Oct 24, 2026
 *      Author: raffi
 *
 *  The commands of the PC: the wheel reads the files the PC sends over the file
 *  link (usb_msg.h), acts on them, and answers. The contents are in
 *  wheel_file_command.h.
 *
 *    settings file  copied into the RAM image of the settings (wheel_settings.h).
 *                   Each group says when it applies (wheel_file_settings.h).
 *                   The ACK of the link is its only answer
 *    COMMAND        carried out, then answered: first by the files it asks for,
 *                   all of them or none (WHEEL_COMMAND_LINK_FULL), last by a
 *                   COMMAND_STATUS that carries its id
 *    any other      dropped
 *
 *  The files are handled one at a time, in the order they came: a command sees
 *  every settings file sent before it. A command the wheel does not know never
 *  gets here: the link refuses it (NACK_BAD_VALUE, see wheel_file_check()).
 *
 *  Main only, from wheel_task(): nothing is handled during the startup, the
 *  files wait in the buffer of the link until it is over
 */

#ifndef COMMAND_WHEEL_COMMAND_H_
#define COMMAND_WHEEL_COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_def.h"

// wheel: the handle of the wheel, kept by the module. The force goes to 0
// through its actuator before a save and before a reset. settings_changed:
// called once the RAM image changed, by a settings file or a revert.
// Main, once, after the modules and the ffb objects exist
void wheel_command_init(Wheel_HandleTypeDef *wheel,
    void (*settings_changed)(void));

// Handles the oldest file of the PC, if one waits. A command whose status does
// not fit in the link yet is taken again by the next call, its action already
// done. Main only, from wheel_task()
void wheel_command_task(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMAND_WHEEL_COMMAND_H_ */
