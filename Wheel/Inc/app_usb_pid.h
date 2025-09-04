/*
 * app_usb_pid.h
 *
 *  Created on: Sep 1, 2025
 *      Author: raffi
 */

#ifndef INC_APP_USB_PID_H_
#define INC_APP_USB_PID_H_

#include "common_types.h"
#include "usbd_def.h"
#include "app_hid_desc.h"

#define GET_REPORT 0x01
#define SET_REPORT 0x09
#define INPUT_REPORT 0x01
#define OUTPUT_REPORT 0x02
#define FEATURE_REPORT 0x03

uint8_t USB_HID_PID_CTL_PARSER(USBD_SetupReqTypedef *req);

#endif /* INC_APP_USB_PID_H_ */
