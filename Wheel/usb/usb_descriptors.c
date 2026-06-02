/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "common_types.h"
#include "ffb/ffb_c.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define PID_MAP(itf, n) ((CFG_TUD_##itf) ? (1 << (n)) : 0)

#define USB_VID   0x1209    /* pid.codes community VID - use your own!  */
#define USB_PID   0xFFB0    /* OpenFFBoard's PID; pick your own product */

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
static tusb_desc_device_t const desc_device = {
  .bLength         = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB          = 0x0200,
  .bDeviceClass    = 0x00,
  .bDeviceSubClass = 0x00,
  .bDeviceProtocol = 0x00,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

  .idVendor  = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = 0x0100,

  .iManufacturer = 0x01,
  .iProduct      = 0x02,
  .iSerialNumber = 0x03,

  .bNumConfigurations = 0x01};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    uint16_t len;
    return ffb_descriptor_1axis(&len);
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum { ITF_NUM_HID = 0, ITF_NUM_TOTAL };

/* FFB needs BOTH an IN endpoint (status reports to the host) and an OUT
 * endpoint (effect reports from the host), so we MUST use the IN+OUT HID
 * template - the plain IN-only TUD_HID_DESCRIPTOR will not work. */
#define EPNUM_HID_OUT   0x01    /* host -> device (effect data)         */
#define EPNUM_HID_IN    0x81    /* device -> host (PID state reports)   */

/* The HID report descriptor length MUST equal what the library returns:
 *   ffb_descriptor_1axis() -> 1196 bytes
 *   ffb_descriptor_2axis() -> 1215 bytes
 * Keep this in sync with the descriptor you return above. */
#define FFB_HID_REPORT_DESC_LEN   1196

#define CONFIG_TOTAL_LEN   (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

uint8_t const desc_configuration[] = {
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
  /* HID IN+OUT interface:
       * itf number, string index, boot protocol, report-desc length,
       * EP OUT addr, EP IN addr, EP size, polling interval (ms).
       * interval 1 ms => 1000 Hz, the FFB default. */
      TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
                               FFB_HID_REPORT_DESC_LEN,
                               EPNUM_HID_OUT, EPNUM_HID_IN,
                               CFG_TUD_HID_EP_BUFSIZE, 1)
  };

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // for multiple configurations
  return desc_configuration;
}

static char const* string_desc_arr[] = {
    (const char[]){ 0x09, 0x04 },   /* 0: language = English (0x0409)   */
    "Open FFBoard",                 /* 1: Manufacturer                  */
    "FFB Wheel",                    /* 2: Product                       */
    "000001",                       /* 3: Serial - ideally from chip ID */
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL;
        }
        const char* str = string_desc_arr[index];
        chr_count = (uint8_t) strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    /* first byte = total length (incl. header), second = string type */
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
