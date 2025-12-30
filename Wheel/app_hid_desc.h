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
 * app_hid_desc.h
 *
 *  Created on: Sep 1, 2025
 *      Author: raffi
 */

#ifndef INC_APP_HID_DESC_H_
#define INC_APP_HID_DESC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "usbd_conf.h"

#define HID_JOYSTICK_REPORT_ID 0x01
#define HID_FfbOnCreateNewEffect_REPORT_ID 0x05
#define HID_FfbOnPIDBlockLoad_REPORT_ID 0x06
#define HID_FfbOnPIDPool_REPORT_ID 0x07
#define HID_FfbOnPIDStatus_REPORT_ID 0x02

extern uint8_t HID_report_desc[USBD_CUSTOM_HID_REPORT_DESC_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* INC_APP_HID_DESC_H_ */
