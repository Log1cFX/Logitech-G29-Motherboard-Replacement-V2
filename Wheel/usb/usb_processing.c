/*
 * usb_processing.c
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 */

#include "wheel_def.h"

USB_State_HandleTypeDef usb_state;
extern Wheel_HandleTypeDef wheel;

void usb_start_deferred_processing() {
	usb_state.processing_state = USB_IS_PROCESSING;
	if (wheel_get_all_component_states() == WHEEL_ERROR) {
#ifdef DEBUG
		Error_Handler();
#endif
#ifdef RELEASE
		wheel.wheel_error_count++;
#endif
	}
	usb_state.report_state = USB_REPORT_IS_READY;
	__HAL_GPIO_EXTI_GENERATE_SWIT(SEND_REPORT_SWIT_PIN);
}

/*
 * returns the encoded direction of the d_pad with the 4 most important bits
 * of the parameter byte
 */
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


void usb_hid_send_report() {
	if (usb_state.report_state == USB_REPORT_NOT_READY) {
		return;
	}
	uint8_t tx[REPORT_SIZE];
	// fill the first byte with d_pad buttons and the first 4 buttons
	tx[0] = 0;
	tx[0] |= 0x0F
			& hat_switch_from_msb((uint8_t) wheel.hButtons->buttons_state);
	tx[0] |= 0xF0 & (wheel.hButtons->buttons_state << 4);
	// set the next buttons
	tx[1] = wheel.hButtons->buttons_state >> 8;
	tx[2] = wheel.hButtons->buttons_state >> 16;
	// activate one of 7 buttons depending on the shifter's speed
	tx[3] = 1U << wheel.hShifter->gear;
	// set the steering axis
	tx[4] = wheel.hSensor->virtual_axis;
	tx[5] = wheel.hSensor->virtual_axis >> 8;
	// set the pedals
	tx[6] = wheel.hPedals->clutch;
	tx[7] = wheel.hPedals->brake;
	tx[8] = wheel.hPedals->throtle;

	tud_hid_report(JOYSTICK_REPORT_ID, tx, REPORT_SIZE-1);

	usb_state.report_state = USB_REPORT_NOT_READY;
}
