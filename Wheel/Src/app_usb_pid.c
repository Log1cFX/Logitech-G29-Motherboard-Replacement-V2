/*
 * app_usb_pid.c
 *
 *  Created on: Sep 1, 2025
 *      Author: raffi
 */

#include "ffb.h"
#include "app_usb_pid.h"
#include "app_usb_hid.h"

uint8_t USB_HID_PID_CTL_PARSER(USBD_SetupReqTypedef *req) {
	uint8_t ret = 0;
	switch (req->bRequest) {
	case GET_REPORT:
		switch (HIBYTE(req->wValue)) {
		case FEATURE_REPORT:
			switch (LOBYTE(req->wValue)) {
			case HID_FfbOnPIDBlockLoad_REPORT_ID:
				FfbOnPIDBlockLoad();
				ret = 1;
				break;
			case HID_FfbOnPIDPool_REPORT_ID:
				FfbOnPIDPool();
				ret = 1;
				break;
			}
			break;
		case INPUT_REPORT:
			switch (LOBYTE(req->wValue)) {
			case HID_FfbOnPIDStatus_REPORT_ID:
				FfbOnPIDStatus();
				ret = 1;
				break;
			}
			break;
		}
		break;
	case SET_REPORT:
		switch (HIBYTE(req->wValue)) {
		case FEATURE_REPORT:
			switch (LOBYTE(req->wValue)) {
			case HID_FfbOnCreateNewEffect_REPORT_ID:
				FfbOnCreateNewEffect(NULL);
				ret = 1;
				break;
			}
			break;
		}
		break;
	}
	return ret;
}
