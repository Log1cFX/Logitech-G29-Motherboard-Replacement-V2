/*
 * app_usb_hid.c
 *
 *  Created on: Aug 7, 2025
 *      Author: raffi
 */

#include "app_usb_hid.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern Wheel_HandleTypeDef wheel;

static uint8_t app_usb_hid_SOF_CB(USBD_HandleTypeDef *pdev);
static uint8_t app_usb_hid_DataInStage_CB(USBD_HandleTypeDef *pdev,
		uint8_t epnum);

Wheel_Status app_usb_hid_init(USB_HID_HandleTypeDef *hUsbHid) {
	if(hUsbHid==0){
		return WHEEL_ERROR;
	}
	hUsbHid->usb_device = &hUsbDeviceFS;
	return WHEEL_OK;
}

Wheel_Status app_usb_hid_start() {
	wheel.hUsbHid->usb_device->pClass->SOF = app_usb_hid_SOF_CB;
	wheel.hUsbHid->usb_device->pClass->DataIn = app_usb_hid_DataInStage_CB;
	return WHEEL_OK;
}

Wheel_Status app_usb_hid_stop() {
	wheel.hUsbHid->usb_device->pClass->SOF = NULL;
	wheel.hUsbHid->usb_device->pClass->DataIn = NULL;
	return WHEEL_OK;
}

void app_usb_hid_deferred_processing() {
	wheel.hUsbHid->processing_state = USB_IS_PROCESSING;
	if(wheel_get_all_component_states() == WHEEL_ERROR){
		Error_Handler();
	}
	wheel.hUsbHid->report_state = USB_REPORT_IS_READY;
	__HAL_GPIO_EXTI_GENERATE_SWIT(wheel.hSwit.usb_send_report_swit_pin);
}

void app_usb_hid_send_report() {
	if (wheel.hUsbHid->report_state == USB_REPORT_NOT_READY) {
		return;
	}
	uint8_t *tx = wheel.hUsbHid->tx_buffer;
	tx[0] = wheel.hButtons->buttons_state;
	tx[1] = wheel.hButtons->buttons_state >> 8;
	tx[2] = wheel.hButtons->buttons_state >> 16;
	tx[3] = wheel.hSensor->virtual_axis;
	tx[4] = wheel.hSensor->virtual_axis >> 8;
	tx[5] = wheel.hPedals->clutch;
	tx[6] = wheel.hPedals->brake;
	tx[7] = wheel.hPedals->throtle;
	USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, tx,
	USBD_CUSTOMHID_INREPORT_BUF_SIZE);
	wheel.hUsbHid->report_state = USB_REPORT_NOT_READY;
}

/* 		CALLBACKS 		*/
// Replaces the dynamically called SOF function of the USB interface
// Is called when a Start Of Frame packet is recieved (1KHz)
// Used to check how many milliseconds the deferred processing wasn't active, for error checking
static uint8_t app_usb_hid_SOF_CB(USBD_HandleTypeDef *pdev) {
	if (wheel.hUsbHid->processing_state == USB_IS_PROCESSING) {
		wheel.hUsbHid->processing_state = USB_NOT_PROCESSING;
		wheel.hUsbHid->unsent_reports = 0;
		return USBD_OK;
	} else {
		wheel.hUsbHid->unsent_reports++;
		if (wheel.hUsbHid->unsent_reports
				>= DEFERRED_PROCESSING_UNSENT_REPORT_THRESHOLD) {
			wheel.hUsbHid->error_count++;
			__HAL_GPIO_EXTI_GENERATE_SWIT(wheel.hSwit.usb_process_data_pin);
		}
		return USBD_OK;
	}
}

// Replaces the dynamically called DataIn function of the USB interface
// Is called when the EP finishes successful a Tx transfer
// Creates an interrupt on the EXTI line
static uint8_t app_usb_hid_DataInStage_CB(USBD_HandleTypeDef *pdev,
		uint8_t epnum) {
	((USBD_CUSTOM_HID_HandleTypeDef*) pdev->pClassData)->state =
			CUSTOM_HID_IDLE; // Enable the next report to be sent
	__HAL_GPIO_EXTI_GENERATE_SWIT(wheel.hSwit.usb_process_data_pin); // Create a software interrupt for deferred processing
	return USBD_OK;
}
