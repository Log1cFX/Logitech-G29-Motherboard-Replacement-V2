/*
 * usb_internal.h
 *
 *  Shared by the files of the USB module only. The interface of the module is
 *  usb_processing.h
 */

#ifndef USB_USB_INTERNAL_H_
#define USB_USB_INTERNAL_H_

#include "tusb.h"
#include "usb_processing.h"

// Report id of the input report (buttons, shifter, steering, pedals)
#define JOYSTICK_REPORT_ID 1

/* FROM usb_callbacks.c TO usb_processing.c */

// The host read the report that was waiting on the endpoint
void usb_on_report_sent(void);
// The ffb library given to usb_init()
ffb_lib_t* usb_ffb(void);

#endif /* USB_USB_INTERNAL_H_ */
