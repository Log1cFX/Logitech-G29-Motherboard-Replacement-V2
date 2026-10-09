/*
 * usb_processing.h
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 *
 *  The USB module: the only part of the firmware that talks to TinyUSB.
 *  Everything it does happens in main, inside usb_task():
 *
 *    wheel --> usb_set_input() --> usb_task() --> host
 *    host --> usb_task() --> ffb library <-- wheel (ffb_calculate, ...)
 *
 *  It knows nothing of the wheel modules: it gets a copy of the controls to
 *  send (wheel_input_t) and the ffb library to hand the host's reports to.
 */

#ifndef USB_USB_PROCESSING_H_
#define USB_USB_PROCESSING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_input.h"
#include "ffb/ffb_c.h"

typedef enum {
  // Not configured: nothing is plugged in, or the host is still enumerating
  USB_DETACHED,
  // Configured, but the HID driver of the host has not read anything yet.
  // A report waits on the endpoint: once it is read, the driver is there
  USB_WAIT_HOST,
  // The host reads the reports
  USB_READY,
  // The bus is suspended: the host sleeps. An unplugged cable also ends up
  // here, the board cannot tell the difference
  USB_SUSPENDED
} usb_state_t;

// Sets up the USB peripheral and starts TinyUSB. Call once, with the ffb
// library already created. custom_task is optional: every usb_task() calls it
void usb_init(ffb_lib_t *ffb, void (*custom_task)(usb_state_t *state));

// Handles what the host sent, updates the state and sends what is waiting.
// Call from main as often as possible, including inside every wait
void usb_task(void);

usb_state_t usb_state(void);

// Copies the controls for usb_task() to send. Only the latest copy is sent
void usb_set_input(const wheel_input_t *input);

// Call from the two USB interrupt handlers only
void usb_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_PROCESSING_H_ */
