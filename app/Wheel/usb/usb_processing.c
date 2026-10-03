/*
 * usb_processing.c
 *
 *  Created on: May 31, 2026
 *      Author: raffi
 */

#include "usb_processing.h"
#include "wheel_def.h"

// if no report got sent for more than this many SysTick ticks (ms), force one
#define REPORT_FORCE_SEND_TICKS 5
#define MICROS_PER_MS 1000

/*
 * CONTEXTS (see the table in wheel_def.h)
 * usb_process_report_data() runs in SysTick (priority 6).
 * usb_send_report() runs in the EXTI0 software interrupt (priority 5).
 * EXTI0 is triggered from two places : by usb_process_report_data(), after it
 * finished updating the component states, and by tud_hid_report_complete_cb(),
 * which runs inside tud_task() (main thread, or SysTick before it updates the
 * component states). As long as nothing else triggers EXTI0, usb_send_report()
 * doesn't run in the middle of an update.
 */

// written by: SysTick (usb_process_report_data), EXTI0 (usb_send_report)
static volatile int8_t unsent_report_cnt;
// written by: the TinyUSB callbacks, through set_hid_driver_state(). They run
// inside tud_task(), so in the main thread, or in SysTick while the driver isn't ready
// read by: SysTick, main thread (init_usb)
static volatile int8_t driver_state;
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

// called every milisecond from SysTick_Handler
void usb_process_report_data() {
	wheel_get_all_component_states();
	if (++unsent_report_cnt > REPORT_FORCE_SEND_TICKS) {
		__HAL_GPIO_EXTI_GENERATE_SWIT(SEND_REPORT_SWIT_PIN);
	}
}

// called after the current report gets sent
// context: EXTI0 software interrupt (priority 5)
void usb_send_report() {
	unsent_report_cnt = -1;

	uint8_t tx[REPORT_SIZE] = { 0 };
	// fill the first byte with d_pad buttons and the first 4 buttons
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
	tx[6] = wheel.hPedals->throtle;
	tx[7] = wheel.hPedals->brake;
	tx[8] = wheel.hPedals->clutch;

	tud_hid_report(JOYSTICK_REPORT_ID, tx, REPORT_SIZE);
}

uint32_t get_faketime_micros() {
	return HAL_GetTick() * MICROS_PER_MS;
}

void set_hid_driver_state(uint8_t en){
	driver_state = en;
}

uint8_t hid_driver_ready(){
	return driver_state & tud_ready();
}

// send the DirectInput PID State input report
bool ffb_send_report_cb(const uint8_t *report, uint16_t len) {
	// report already contains id, so use id 0
	return tud_hid_report(0, report, len);
}
