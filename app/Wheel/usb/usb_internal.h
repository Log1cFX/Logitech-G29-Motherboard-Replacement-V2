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
#include "usb_msg.h"

// The two HID interfaces, as TinyUSB numbers them: in the order of the
// configuration descriptor. Its calls that take no number use USB_HID_WHEEL
#define USB_HID_WHEEL 0 // input reports and force feedback
#define USB_HID_FILES 1 // the file link, see usb_msg.h

// Report id of the input report (buttons, shifter, steering, pedals)
#define JOYSTICK_REPORT_ID 1

/* FROM usb_callbacks.c TO usb_processing.c */

// The host read the report that was waiting on the endpoint
void usb_on_report_sent(void);
// The ffb library given to usb_init()
ffb_lib_t* usb_ffb(void);

/* FROM usb_msg.c */

// The part of the file link that moves blocks, see PROTOCOL in usb_msg.c.
// A block is one report of USB_HID_FILES, which has no report id.
// now_ms is always HAL_GetTick()
#define USB_MSG_BLOCK_SIZE 64

// Empties the buffer and closes the connection. Before any other call
void usb_msg_init(void);
// Runs the timeouts of the protocol. Call at every usb_task()
void usb_msg_task(uint32_t now_ms);
// Builds the next block for the PC: an answer, or the next bytes of the file
// in flight. False when there is nothing to send. Changes nothing: calling it
// again builds the same block
bool usb_msg_next_block(uint8_t block[USB_MSG_BLOCK_SIZE]);
// The block of the last usb_msg_next_block() was given to TinyUSB
void usb_msg_block_sent(uint32_t now_ms);
// A block of the PC. len: bytes received, the first byte of the block included
void usb_msg_on_block(const uint8_t *block, uint16_t len, uint32_t now_ms);
// The bus left the configured state: whoever was connected is gone
void usb_msg_link_down(void);

#endif /* USB_USB_INTERNAL_H_ */
