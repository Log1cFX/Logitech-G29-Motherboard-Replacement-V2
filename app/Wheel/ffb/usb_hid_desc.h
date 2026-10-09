/*
 * usb_hid_desc.h
 *
 *  Created on: 7 oct. 2026
 *      Author: raffi
 *
 *  HID report descriptor of the wheel, built in usb_hid_desc.cpp
 */

#ifndef FFB_USB_HID_DESC_H_
#define FFB_USB_HID_DESC_H_

#include <stdint.h>

// Size of hid_g29_desc_bytes, checked at build time in usb_hid_desc.cpp
#define HID_G29_DESC_LEN 1225

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t hid_g29_desc_bytes[];

#ifdef __cplusplus
}
#endif

#endif /* FFB_USB_HID_DESC_H_ */
