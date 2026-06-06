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
#include <stdbool.h>

void usb_process_report_data();
void usb_send_report();
uint32_t get_faketime_micros();
void set_hid_driver_state(uint8_t en);
uint8_t hid_driver_ready();
bool ffb_send_report_cb(const uint8_t *report, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_PROCESSING_H_ */
