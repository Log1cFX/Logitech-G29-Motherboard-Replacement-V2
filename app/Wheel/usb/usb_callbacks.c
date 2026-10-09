/*
 * usb_callbacks.c
 *
 *  Created on: May 30, 2026
 *      Author: raffi
 *
 *  The TinyUSB callbacks of the USB module. TinyUSB calls them from tud_task(),
 *  so from usb_task(), in main (see usb_processing.c)
 */

#include "usb_internal.h"

// The host read the report that was waiting on the IN endpoint
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report,
                                uint16_t len) {
  (void)instance;
  (void)report;
  (void)len;

  usb_on_report_sent();
}

// GET_REPORT request. TinyUSB already wrote the report id: buffer starts right
// after it. Returns the number of bytes written, 0 stalls the request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_type;

  return ffb_hid_get(usb_ffb(), report_id, buffer, reqlen);
}

// SET_REPORT request, or data received on the OUT endpoint
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  (void)instance;

  // For SET_REPORT, TinyUSB gives the report id and a buffer without it.
  // For the OUT endpoint, it gives id 0 and the whole buffer: the id is then
  // its first byte (same fix-up as OpenFFBoard)
  if ((report_type == HID_REPORT_TYPE_INVALID
      || report_type == HID_REPORT_TYPE_OUTPUT)
      && report_id == 0) {
    if (bufsize > 0) report_id = buffer[0];
  }
  ffb_hid_out(usb_ffb(), report_id, buffer, bufsize);
}
