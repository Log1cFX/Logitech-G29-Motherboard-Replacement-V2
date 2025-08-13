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
static uint8_t hat_switch_from_msb(uint8_t byte);

Wheel_Status app_usb_hid_init(USB_HID_HandleTypeDef *hUsbHid) {
	if (hUsbHid == 0) {
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

/* 		FUNCTIONS COMPOSING THE REPORT 		*/

void app_usb_hid_deferred_processing() {
	wheel.hUsbHid->processing_state = USB_IS_PROCESSING;
	if (wheel_get_all_component_states() == WHEEL_ERROR) {
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
	tx[0] = 0;
	tx[0] |= 0x0F & hat_switch_from_msb(wheel.hButtons->buttons_state);
	tx[0] |= 0xF0 & (wheel.hButtons->buttons_state << 4);
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

static uint8_t hat_switch_from_msb(uint8_t byte) {
	/* Extract individual direction bits (boolean 0 / 1) */
	uint8_t down = (byte & 0x10u) ? 1u : 0u; // bit 4
	uint8_t left = (byte & 0x20u) ? 1u : 0u; // bit 5
	uint8_t up = (byte & 0x40u) ? 1u : 0u; // bit 6
	uint8_t right = (byte & 0x80u) ? 1u : 0u; // bit 7

	// Convert to signed axis values −1 / 0 / +1
	int8_t dpadX = (int8_t) right - (int8_t) left; // +1 = Right, −1 = Left
	int8_t dpadY = (int8_t) down - (int8_t) up; // +1 = Down,  −1 = Up

	// If both opposite directions are pressed, cancel the axis
	if ((right && left))
		dpadX = 0;
	if ((up && down))
		dpadY = 0;

	/* Map (dpadX, dpadY) to the hat-switch look-up table */
	static const uint8_t hatTable[3][3] = {
	/*           X = −1   0   +1  */
	/* Y = −1 */{ 7, 0, 1 }, /* Up, Up-Left(NW), Up-Right(NE) */
	/* Y =  0 */{ 6, 8, 2 }, /* Left, Centre,   Right        */
	/* Y = +1 */{ 5, 4, 3 } /* Down-Left(SW), Down, Down-Right(SE) */
	};
	return hatTable[dpadY + 1][dpadX + 1];
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
