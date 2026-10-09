/*
 * usb_processing.h
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 *
 *  The USB module.
 *
 *  It is the only part of the firmware that talks to TinyUSB, and everything
 *  it does happens in the main thread, inside usb_task(). The rest of the
 *  firmware only uses the functions below :
 *
 *    main code --> usb_set_input() --> usb_task() --> host
 *    host --> usb_task() --> ffb library <-- main code (ffb_calculate, ...)
 *
 *  The module doesn't know the hardware modules of the wheel. It only gets a
 *  copy of the state of the controls (wheel_input_t), and the handle of the
 *  ffb library to give it what the host sends.
 */

#ifndef USB_USB_PROCESSING_H_
#define USB_USB_PROCESSING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_input.h"
#include "ffb/ffb_c.h"

typedef enum {
  // Not configured by a host : nothing is plugged in, or the host is still
  // enumerating the device
  USB_DETACHED,
  // Configured, but the HID driver of the host hasn't read anything from us
  // yet. A report is waiting on the endpoint, the day the host reads it we
  // know that the driver is there
  USB_WAIT_HOST,
  // The host reads our reports
  USB_READY,
  // The bus is suspended : the host is asleep. A cable that gets unplugged
  // also ends up here, the board has no way to tell the difference
  USB_SUSPENDED
} usb_state_t;

// Sets up the USB peripheral and starts TinyUSB. To call once, when the ffb
// library is created : the module gives it everything the host sends
void usb_init(ffb_lib_t *ffb, void (*custom_task)(usb_state_t *state));

// Does all the work : handles what the host sent, updates the state and sends
// what is waiting. Additionally, calls the custom task if it was provided.
// To call from the main thread, as often as possible, and also inside every
// wait of the main thread (the host has to be answered)
void usb_task(void);

usb_state_t usb_state(void);

// Gives the module the latest state of the controls. It is copied, and sent to
// the host by usb_task() when possible. If it is called several times before
// the host reads, the host only gets the latest one
void usb_set_input(const wheel_input_t *input);

// To call from the two USB interrupt handlers, and from nowhere else
void usb_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_PROCESSING_H_ */
