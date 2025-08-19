/*
 * app_usb_hid.h
 *
 *  Created on: Aug 7, 2025
 *      Author: raffi
 */

#ifndef SRC_APP_USB_HID_H_
#define SRC_APP_USB_HID_H_

#include "steeringwheel.h"
#include "usbd_conf.h"
#include "usb_device.h"
#include "usbd_customhid.h"

#define USBD_CUSTOMHID_INREPORT_BUF_SIZE 9
#define DEFERRED_PROCESSING_UNSENT_REPORT_THRESHOLD 25

typedef enum {
	USB_REPORT_NOT_READY, USB_REPORT_IS_READY
} Report_State;

typedef enum {
	USB_NOT_PROCESSING, USB_IS_PROCESSING
} Report_Processing_State;

typedef struct _USB_HID_HandleTypeDef {
	uint8_t tx_buffer[USBD_CUSTOMHID_INREPORT_BUF_SIZE];
	uint8_t rx_buffer[0];
	Report_State report_state;
	Report_Processing_State processing_state;
	uint8_t unsent_reports;
	uint16_t error_count;
	USBD_HandleTypeDef *usb_device;
} USB_HID_HandleTypeDef;

Wheel_Status app_usb_hid_init(USB_HID_HandleTypeDef *husb);
Wheel_Status app_usb_hid_start();
Wheel_Status app_usb_hid_stop();
void app_usb_hid_deferred_processing();
void app_usb_hid_send_report();

#endif /* SRC_APP_USB_HID_H_ */
