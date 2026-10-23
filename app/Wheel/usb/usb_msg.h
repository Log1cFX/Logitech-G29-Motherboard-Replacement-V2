/*
 * usb_msg.h
 *
 *  Created on: Oct 13, 2026
 *      Author: raffi
 *
 *  The file link to the PC: files (wheel_file.h) exchanged with an application
 *  of the PC over interface 1 of the device, each one acknowledged by its
 *  receiver. The interface is of the vendor class, with two bulk endpoints:
 *  no input API of the PC sees it. Windows binds WinUSB to it by itself (see
 *  MICROSOFT OS 2.0 in usb_descriptors.c). The PC opens it with libusb, or
 *  finds it with the WinUSB API by its interface GUID. One application at a
 *  time.
 *
 *    wheel --> usb_msg_send() --> buffer --> usb_task() --> PC
 *    PC --> usb_task() --> buffer --> usb_msg_peek(), usb_msg_pop() --> wheel
 *
 *  One buffer of the module holds the files of both directions, each direction
 *  in the order of arrival.
 *
 *    to the PC    a file waits in the buffer until the PC acknowledges it. So
 *                 it can be sent with no application connected: it leaves
 *                 after the next sync
 *    from the PC  a file is only seen once it is complete and passed
 *                 wheel_file_check(). Until then the PC gets no acknowledgment
 *
 *  The protocol and the buffer are described in usb_msg.c.
 *
 *  Main only, after usb_init(). Nothing here blocks
 */

#ifndef USB_USB_MSG_H_
#define USB_USB_MSG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "wheel_file.h"

// Shared by the files of both directions. Each one takes 4 bytes more than its
// WHEEL_FILE_TOTAL_SIZE
#define USB_MSG_BUFFER_SIZE 2048
// Biggest content of a file: one that fills its flash page, see
// WHEEL_FILE_MAX_CONTENT_SIZE
#define USB_MSG_MAX_CONTENT_SIZE 1016

typedef enum {
  USB_MSG_OK, USB_MSG_FULL, // no free place of the buffer is big enough: nothing is stored
  USB_MSG_INVALID// content over USB_MSG_MAX_CONTENT_SIZE
}usb_msg_status_t;

// Builds a file for the PC in the buffer, around a copy of the content: see
// wheel_file_make(). The content is no longer needed once this returns.
// USB_MSG_OK means stored, not delivered: the file leaves the buffer when the
// PC acknowledges it, or refuses it
usb_msg_status_t usb_msg_send(uint8_t type, const void *content, uint16_t size);

// Oldest file of the PC, NULL when none waits. It lies in the buffer of the
// module, on a multiple of 4 bytes: its content can be cast to the struct of
// its type. Valid until usb_msg_pop()
const wheel_file_t* usb_msg_peek(void);

// Removes the file of usb_msg_peek(), which then returns the next one
void usb_msg_pop(void);

// True from the sync of an application of the PC until it stops
// acknowledging, or until the bus is lost
bool usb_msg_connected(void);

// True while a file for the PC with this crc field waits in the buffer: the PC
// has neither acknowledged nor refused it yet
bool usb_msg_pending(uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_MSG_H_ */
