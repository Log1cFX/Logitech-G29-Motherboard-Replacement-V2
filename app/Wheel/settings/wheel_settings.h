/*
 * wheel_settings.h
 *
 *  Created on: Oct 20, 2026
 *      Author: raffi
 *
 *  The settings of the wheel: one file per group (wheel_file_settings.h), back
 *  to back in a page of the flash and in its image in RAM.
 *
 *    settings page (flash) --> wheel_settings_init() --> RAM image --> users
 *    command module --> wheel_settings_receive() --> RAM image
 *    RAM image --> wheel_settings_save() --> backup page, then settings page
 *
 *  THE PAGE
 *  Linked at WHEEL_FILE_PAGE_SETTINGS (.settings in the linker script): its
 *  values are the ones the firmware is flashed with, written as the initializer
 *  of the image (wheel_settings_image.c). There are no other values: the PC
 *  application brings back the original ones when it wants to.
 *  The backup page holds the same bytes. A save writes it first, then the
 *  settings page: a power loss always leaves one of them valid, and the next
 *  start makes the other one equal to it again.
 *
 *  THE IMAGE
 *  Loaded from the settings page at the start, like .data, just above .bss.
 *  Garbage before wheel_settings_init(). Then main reads it through
 *  wheel_settings, and the modules through the pointer their config holds:
 *  they see a change at their next read. Only main writes it, one whole file at
 *  a time with the interrupts off, and only with files that passed
 *  wheel_file_check(): a reader never checks a value again
 */

#ifndef SETTINGS_WHEEL_SETTINGS_H_
#define SETTINGS_WHEEL_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_file.h"
#include <stdbool.h>

// One settings file as it lies in the page: its header, then its content
#define WHEEL_SETTINGS_FILE(content_t) \
  struct { \
    wheel_file_header_t header; \
    content_t content; \
  }

// The page, and the image. Each file starts on a multiple of 4 bytes and has no
// padding: the contents are multiples of 4 bytes
typedef struct {
  WHEEL_SETTINGS_FILE(wheel_file_settings_ffb_t)
  ffb;
  WHEEL_SETTINGS_FILE(wheel_file_settings_steering_t)
  steering;
  WHEEL_SETTINGS_FILE(wheel_file_settings_calibration_t)
  calibration;
  WHEEL_SETTINGS_FILE(wheel_file_settings_shifter_t)
  shifter;
  WHEEL_SETTINGS_FILE(wheel_file_settings_pedals_t)
  pedals;
  WHEEL_SETTINGS_FILE(wheel_file_settings_buttons_t)
  buttons;
  WHEEL_SETTINGS_FILE(wheel_file_settings_system_t)
  system;
  WHEEL_SETTINGS_FILE(wheel_file_settings_force_t)
  force;
}wheel_settings_page_t;

// The settings in use: the RAM image, after wheel_settings_init(). Read-only
// outside wheel_settings.c. Main, and the modules through their pointers
extern const wheel_settings_page_t *const wheel_settings;

// Checks both pages and makes them equal again: repairs a corrupted one, or
// refills the backup when it differs. Then copies the settings page into the
// image. Fatal (WHEEL_ERR_SETTINGS) when neither page is valid. Main, in
// wheel_startup(), right after wheel_log_init()
void wheel_settings_init(void);

/* FOR THE COMMAND MODULE */
// The settings file of this type in the image, NULL for any other type. Its
// content can be given to usb_msg_send()
const wheel_file_t* wheel_settings_file(uint8_t type);

// Copies one settings file, already checked by wheel_file_check(), over its
// place in the image, interrupts off. Flash is untouched. Main only
void wheel_settings_receive(const wheel_file_t *file);

// Writes the image to the backup page, then to the settings page. False if the
// flash failed before the settings page changed. Main only. Stalls up to about
// 100 ms with the interrupts blocked: the caller sets the force to 0 first
bool wheel_settings_save(void);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_WHEEL_SETTINGS_H_ */
