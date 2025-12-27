/*
 * hid_desc.h
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
