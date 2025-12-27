/*
 * app_usb_hid.c
 *
 *  Created on: Aug 7, 2025
 *      Author: raffi
 */

#include "app_usb_hid_pid.h"
#include "app_hid_desc.h"
#include "ffb.h"
#include "ffb.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern Wheel_HandleTypeDef wheel;

static uint8_t app_usb_SOF_CB(USBD_HandleTypeDef *pdev);
static uint8_t app_usb_DataInStage_CB(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t app_usb_DataOutStage_CB(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t app_usb_EP0_recieve_CB(USBD_HandleTypeDef *pdev);
static uint8_t hat_switch_from_msb(uint8_t byte);

Wheel_Status app_usb_hid_init(USB_HID_HandleTypeDef *hUsbHid) {
	if (hUsbHid == 0) {
		return WHEEL_ERROR;
	}
	hUsbHid->usb_device = &hUsbDeviceFS;
	return WHEEL_OK;
}

Wheel_Status app_usb_start() {
	wheel.hUsbHid->usb_device->pClass->SOF = app_usb_SOF_CB;
	wheel.hUsbHid->usb_device->pClass->DataIn = app_usb_DataInStage_CB;
	wheel.hUsbHid->usb_device->pClass->DataOut = app_usb_DataOutStage_CB;
	wheel.hUsbHid->usb_device->pClass->EP0_RxReady = app_usb_EP0_recieve_CB;

	return WHEEL_OK;
}

Wheel_Status app_usb_stop() {
//	wheel.hUsbHid->usb_device->pClass->SOF = NULL;
//	wheel.hUsbHid->usb_device->pClass->DataIn = NULL;
//	((USBD_CUSTOM_HID_ItfTypeDef*) wheel.hUsbHid->usb_device->pUserData)->OutEvent =
//	NULL;
	return WHEEL_OK;
}

/* 		FUNCTIONS COMPOSING THE REPORT 		*/

void app_usb_start_deferred_processing() {
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
	uint8_t *tx = wheel.hUsbHid->Ep1In_buffer;
	// assign the report ID
	tx[0] = HID_JOYSTICK_REPORT_ID;
	// fill the first byte with d_pad buttons and the first 4 buttons
	tx[1] = 0;
	tx[1] |= 0x0F
			& hat_switch_from_msb((uint8_t) wheel.hButtons->buttons_state);
	tx[1] |= 0xF0 & (wheel.hButtons->buttons_state << 4);
	// set the next buttons
	tx[2] = wheel.hButtons->buttons_state >> 8;
	tx[3] = wheel.hButtons->buttons_state >> 16;
	// activate one of 7 buttons depending on the shifter's speed
	tx[4] = 1U << wheel.hShifter->speed;
	// set the steering axis
	tx[5] = wheel.hSensor->virtual_axis;
	tx[6] = wheel.hSensor->virtual_axis >> 8;
	// set the pedals
	tx[7] = wheel.hPedals->clutch;
	tx[8] = wheel.hPedals->brake;
	tx[9] = wheel.hPedals->throtle;
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

	// Map (dpadX, dpadY) to the hat-switch look-up table
	static const uint8_t hat_table[3][3] = {
	/**/{ 7, 0, 1 },/**/
	/**/{ 6, 8, 2 },/**/
	/**/{ 5, 4, 3 }/**/
	};
	return hat_table[dpadY + 1][dpadX + 1];
}

/* 		CALLBACKS 		*/
// Replaces the dynamically called SOF function of the USB interface.
// Is called when a Start Of Frame packet is recieved (1KHz).
// Used to check how many milliseconds the deferred processing wasn't active, for error checking.
static uint8_t app_usb_SOF_CB(USBD_HandleTypeDef *pdev) {
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

// This function is called from the library for additional PID commands.
uint8_t USB_HID_PID_CTL_PARSER(USBD_HandleTypeDef *pdev,
		USBD_SetupReqTypedef *req) {
	USBD_CUSTOM_HID_HandleTypeDef *hhid =
			(USBD_CUSTOM_HID_HandleTypeDef*) pdev->pClassData;
	uint8_t ret = 0;
	switch (req->bRequest) {
	case GET_REPORT:
		switch (HIBYTE(req->wValue)) {
		case FEATURE_REPORT:
			switch (LOBYTE(req->wValue)) {
			case HID_FfbOnPIDBlockLoad_REPORT_ID:
				memcpy(wheel.hUsbHid->Ep0In_buffer, FfbOnPIDBlockLoad(),
						sizeof(USB_FFBReport_PIDBlockLoad_Feature_Data_t));
				USBD_CtlSendData(pdev, wheel.hUsbHid->Ep0In_buffer,
						req->wLength);
				ret = 1;
				break;
			case HID_FfbOnPIDPool_REPORT_ID:
				memcpy(wheel.hUsbHid->Ep0In_buffer, FfbOnPIDPool(),
						sizeof(USB_FFBReport_PIDPool_Feature_Data_t));
				USBD_CtlSendData(pdev, wheel.hUsbHid->Ep0In_buffer,
						req->wLength);
				ret = 1;
				break;
			}
			break;
		case INPUT_REPORT:
			switch (LOBYTE(req->wValue)) {
			case HID_FfbOnPIDStatus_REPORT_ID:
				memcpy(wheel.hUsbHid->Ep0In_buffer, FfbOnPIDStatus(),
						sizeof(USB_FFBReport_PIDStatus_Input_Data_t));
				USBD_CtlSendData(pdev, wheel.hUsbHid->Ep0In_buffer,
						req->wLength);
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
				hhid->IsReportAvailable = 1U;
				wheel.hUsbHid->on_new_effect_request = 1;
				// sets flag to process the received information later
				// (inside app_usb_EP0_recieve_CB)
				USBD_CtlPrepareRx(pdev, wheel.hUsbHid->Ep0Out_buffer,
						req->wLength);
				ret = 1;
				break;
			}
			break;
		}
		break;
	}
	return ret;
}

/*		CALLBACKS		*/

/*
 * Replaces the dynamically called DataIn function of the USB HID interface.
 * Is called when the HID EP finishes successful a Tx transfer.
 * Creates a software interrupt on the EXTI line for deferred usb processing.
 */
static uint8_t app_usb_DataInStage_CB(USBD_HandleTypeDef *pdev, uint8_t epnum) {
	((USBD_CUSTOM_HID_HandleTypeDef*) pdev->pClassData)->state =
			CUSTOM_HID_IDLE; // Enable the next report to be sent
	__HAL_GPIO_EXTI_GENERATE_SWIT(wheel.hSwit.usb_process_data_pin); // Create a software interrupt for deferred processing
	return USBD_OK;
}

/*
 * Replaces the dynamically called DataOut function of the USB HID interface.
 * Is called when the HID EP finishes successful a Rx transfer.
 * Calls the function that processes FFB data then prepares to recieve the next packet.
 */
static uint8_t app_usb_DataOutStage_CB(USBD_HandleTypeDef *pdev, uint8_t epnum) {
	FfbOnUsbData(wheel.hUsbHid->Ep1Out_buffer,
	USBD_CUSTOMHID_OUTREPORT_BUF_SIZE);
	USBD_LL_PrepareReceive(pdev, CUSTOM_HID_EPOUT_ADDR,
			wheel.hUsbHid->Ep1Out_buffer,
			USBD_CUSTOMHID_OUTREPORT_BUF_SIZE);
	return USBD_OK;
}

/*
 * Replaces the dynamically called EP0_RxReady function of the USB HID interface.
 * Is called when the control endpoint finishes successful a Rx transfer.
 * If called after a "HID_FfbOnCreateNewEffect_REPORT_ID" packet,
 * calls the appropriate function to create a new effect.
 */
static uint8_t app_usb_EP0_recieve_CB(USBD_HandleTypeDef *pdev) {
	USBD_CUSTOM_HID_HandleTypeDef *hhid =
			(USBD_CUSTOM_HID_HandleTypeDef*) pdev->pClassData;
	if (hhid->IsReportAvailable == 1U && wheel.hUsbHid->on_new_effect_request) {
		FfbOnCreateNewEffect(
				(USB_FFBReport_CreateNewEffect_Feature_Data_t*) wheel.hUsbHid->Ep0Out_buffer);
		hhid->IsReportAvailable = 0U;
		wheel.hUsbHid->on_new_effect_request = 0;
	}

	return USBD_OK;
}
