/*
 * usb_hid_desc.h
 *
 *  Created on: 7 oct. 2026
 *      Author: raffi
 *
 *  The HID report descriptor of the wheel, for the C code.
 *  The array itself is built in usb_hid_desc.cpp
 */

#ifndef FFB_USB_HID_DESC_H_
#define FFB_USB_HID_DESC_H_

#include <stdint.h>

// Size of hid_g29_desc_bytes in bytes. It is checked against the real array
// in usb_hid_desc.cpp : the build fails if the two ever disagree
#define HID_G29_DESC_LEN 1225

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t hid_g29_desc_bytes[];

#ifdef __cplusplus
}
#endif

#endif /* FFB_USB_HID_DESC_H_ */
