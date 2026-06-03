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

void usb_process_report_data();
void usb_send_report();
uint32_t get_faketime_micros();

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_PROCESSING_H_ */
