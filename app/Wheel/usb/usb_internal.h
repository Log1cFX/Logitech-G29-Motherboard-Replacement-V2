/*
 * usb_internal.h
 *
 *  What the files of the USB module share with each other.
 *  Nothing outside of Wheel/usb should include this, the interface of the
 *  module is usb_processing.h
 */

#ifndef USB_USB_INTERNAL_H_
#define USB_USB_INTERNAL_H_

#include "tusb.h"
#include "usb_processing.h"

// report id of the input report (buttons, shifter, steering, pedals)
#define JOYSTICK_REPORT_ID 1

/* From the TinyUSB callbacks (usb_callbacks.c) to the core (usb_processing.c) */

// a report that was waiting on the endpoint got read by the host
void usb_on_report_sent(void);
// the ffb library that was given to usb_init()
ffb_lib_t* usb_ffb(void);

#endif /* USB_USB_INTERNAL_H_ */
