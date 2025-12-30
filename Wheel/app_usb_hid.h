/*
 MIT License

 Copyright (c) 2025 Log1cFX

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/*
 * app_usb_hid.h
 *
 *  Created on: Aug 7, 2025
 *      Author: raffi
 */

#ifndef INC_APP_USB_HID_H_
#define INC_APP_USB_HID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "wheel_def.h"
#include "usb_device.h"
#include "usbd_conf.h"
#include "usbd_def.h"
#include "usbd_customhid.h"

#define USBD_CUSTOMHID_INREPORT_BUF_SIZE 10
#define DEFERRED_PROCESSING_UNSENT_REPORT_THRESHOLD 25

#define GET_REPORT 0x01
#define SET_REPORT 0x09
#define INPUT_REPORT 0x01
#define OUTPUT_REPORT 0x02
#define FEATURE_REPORT 0x03

typedef enum {
	USB_REPORT_NOT_READY, USB_REPORT_IS_READY
} Report_State;

typedef enum {
	USB_NOT_PROCESSING, USB_IS_PROCESSING
} Report_Processing_State;

typedef struct _USB_HID_HandleTypeDef {
	uint8_t Ep1In_buffer[USBD_CUSTOMHID_INREPORT_BUF_SIZE];
	uint8_t Ep1Out_buffer[USBD_CUSTOMHID_OUTREPORT_BUF_SIZE];
	uint8_t Ep0In_buffer[8];
	uint8_t Ep0Out_buffer[8];
	Report_State report_state;
	Report_Processing_State processing_state;
	uint8_t on_new_effect_request;
	uint32_t unsent_reports;
	uint32_t error_count;
	USBD_HandleTypeDef *usb_device;
} USB_HID_HandleTypeDef;

Wheel_Status app_usb_hid_init(USB_HID_HandleTypeDef *husb);
Wheel_Status app_usb_start();
Wheel_Status app_usb_stop();
void app_usb_start_deferred_processing();
void app_usb_hid_send_report();

uint8_t USB_HID_PID_CTL_PARSER(USBD_HandleTypeDef *pdev,
		USBD_SetupReqTypedef *req);

extern USB_HID_HandleTypeDef hUsbHidPid;

#ifdef __cplusplus
}
#endif

#endif /* INC_APP_USB_HID_H_ */
