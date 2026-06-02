/*
 * usb_callbacks.c
 *
 *  Created on: May 30, 2026
 *      Author: raffi
 */

#include "usb_processing.h"
#include "wheel_def.h"
#include "ffb/ffb_c.h"

extern USB_State_HandleTypeDef usb_state;
extern Wheel_HandleTypeDef wheel;
extern ffb_lib_t* hFFB;

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+


// Invoked when device is mounted
void tud_mount_cb(void) {
	usb_state.usb_connected = 1;
	tud_sof_cb_enable(true); // only works after enumeration
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
	usb_state.usb_connected = 1;
	tud_sof_cb_enable(true); // only works after enumeration
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
	usb_state.usb_connected = 0;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
	(void) remote_wakeup_en;
	usb_state.usb_connected = 0;
}

// Is called when a Start Of Frame packet is received (1KHz).
// Used to check how many milliseconds the deferred processing wasn't active, for error checking.
void tud_sof_cb(uint32_t frame_count) {
	if (usb_state.processing_state == USB_IS_PROCESSING) {
		usb_state.processing_state = USB_NOT_PROCESSING;
		usb_state.unsent_reports = 0;
	} else {
		usb_state.unsent_reports++;
		if (usb_state.unsent_reports
				>= DEFERRED_PROCESSING_UNSENT_REPORT_THRESHOLD) {
			usb_state.error_count++;
//			__HAL_GPIO_EXTI_GENERATE_SWIT(PROCESS_DATA_SWIT_PIN);
		}
	}
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report,
		uint16_t len) {
	(void) instance;
	(void) report;
	(void) len;

	// Create a software interrupt for deferred processing
//	__HAL_GPIO_EXTI_GENERATE_SWIT(PROCESS_DATA_SWIT_PIN);
}


// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
	(void) instance;
	(void) protocol;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
		hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
	// first byte already has the report id
	// what we get passed as argument is buffer[1]
	// we need to fill the buffer (be careful to not duplicate the report_id)
	// then return the length of what we added to the buffer
	return ffb_hid_get(hFFB, report_id, buffer, reqlen);
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
		hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
	/*tinyusb is weird,
	 *for normal out transfers, it puts report_id to 0 and sends the original buffer
	 *for ctrl out set_report, it puts the correct report id and truncates the buffer to remove id*/

	/* The first byte of an OUT report received on the interrupt EP
	 * with no report_id may be the report ID itself - mirror the
	 * original OpenFFBoard fix-up. */
	if ((report_type == HID_REPORT_TYPE_INVALID
			|| report_type == HID_REPORT_TYPE_OUTPUT) && report_id == 0) {
		if (bufsize > 0)
			report_id = buffer[0];
	}
	ffb_hid_out(hFFB, report_id, buffer, bufsize);
}
