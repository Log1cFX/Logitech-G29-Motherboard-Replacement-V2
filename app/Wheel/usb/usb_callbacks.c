/*
 * usb_callbacks.c
 *
 *  Created on: May 30, 2026
 *      Author: raffi
 *
 *  The callbacks of TinyUSB that the USB module uses.
 *  TinyUSB calls all of them from inside tud_task(), so from usb_task(), so
 *  from the main thread (see usb_processing.c).
 */

#include "usb_internal.h"

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when sent REPORT successfully to host
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report,
                                uint16_t len) {
  (void)instance;
  (void)report;
  (void)len;

  usb_on_report_sent();
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_type;

  // first byte already has the report id
  // what we get passed as argument is buffer[1]
  // we need to fill the buffer (be careful to not duplicate the report_id)
  // then return the length of what we added to the buffer
  return ffb_hid_get(usb_ffb(), report_id, buffer, reqlen);
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  (void)instance;

  /*tinyusb is weird,
   *for normal out transfers, it puts report_id to 0 and sends the original buffer
   *for ctrl out set_report, it puts the correct report id and truncates the buffer to remove id*/

  /* The first byte of an OUT report received on the interrupt EP
   * with no report_id may be the report ID itself - mirror the
   * original OpenFFBoard fix-up. */
  if ((report_type == HID_REPORT_TYPE_INVALID
      || report_type == HID_REPORT_TYPE_OUTPUT)
      && report_id == 0) {
    if (bufsize > 0) report_id = buffer[0];
  }
  ffb_hid_out(usb_ffb(), report_id, buffer, bufsize);
}
