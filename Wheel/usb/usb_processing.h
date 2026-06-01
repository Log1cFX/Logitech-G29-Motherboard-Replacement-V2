/*
 * usb_processing.h
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 */

#ifndef USB_USB_PROCESSING_H_
#define USB_USB_PROCESSING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "tusb_config.h"
#include "hid_device.h"

#define DEFERRED_PROCESSING_UNSENT_REPORT_THRESHOLD 25

typedef enum {
	USB_REPORT_NOT_READY, USB_REPORT_IS_READY
} Report_State;

typedef enum {
	USB_NOT_PROCESSING, USB_IS_PROCESSING
} Report_Processing_State;

typedef struct _USB_State {
	Report_State report_state;
	Report_Processing_State processing_state;
	uint8_t on_new_effect_request;
	uint32_t unsent_reports;
	uint32_t error_count;
} USB_State_HandleTypeDef;

void usb_start_deferred_processing();
void usb_hid_send_report();

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_PROCESSING_H_ */
