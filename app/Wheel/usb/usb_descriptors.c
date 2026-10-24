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

#include "usb_internal.h"
#include "usb_hid_desc.h"
#include "wheel_version.h"
#include <assert.h>
#include <string.h>

// The descriptors of the device and the TinyUSB callbacks that return them.
// What a callback returns has to stay valid until the transfer completes

// One PID bit per enabled TinyUSB class. Unused: the PID is fixed below
#define PID_MAP(itf, n) ((CFG_TUD_##itf) ? (1 << (n)) : 0)

#define USB_VID   0x1209 // pid.codes community VID, to replace with our own
#define USB_PID   0xFFB0 // OpenFFBoard's PID, to replace with our own

/* DEVICE DESCRIPTOR */

// @formatter:off
static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB =          0x0210, // 2.1: the host reads the BOS, see MICROSOFT OS 2.0
    .bDeviceClass =    0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor =  USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = WHEEL_VERSION_BCD, // see wheel_version.h

    .iManufacturer = 0x01,
    .iProduct =      0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};
// @formatter:on
uint8_t const*
tud_descriptor_device_cb(void) {
  return (uint8_t const*)&desc_device;
}

/* HID REPORT DESCRIPTOR */

// The wheel is the only HID interface
uint8_t const*
tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return hid_g29_desc_bytes;
}

/* CONFIGURATION DESCRIPTOR */

enum {
  ITF_NUM_HID = 0, // the wheel
  ITF_NUM_FILES, // the file link, see usb_msg.h
  ITF_NUM_TOTAL
};

// FFB needs both endpoints, hence TUD_HID_INOUT_DESCRIPTOR below and not the
// IN-only TUD_HID_DESCRIPTOR
#define EPNUM_HID_OUT   0x01 // host -> device: effects
#define EPNUM_HID_IN    0x81 // device -> host: input and PID state reports
// Bulk: they only take the bandwidth the reports of the wheel leave free
#define EPNUM_FILES_OUT 0x02 // host -> device: blocks of the file link
#define EPNUM_FILES_IN  0x82 // device -> host: blocks of the file link

#define STRID_FILES 0x04 // name of the file link, see STRING DESCRIPTORS

#define FFB_HID_REPORT_DESC_LEN   HID_G29_DESC_LEN

#define CONFIG_TOTAL_LEN   (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN \
                            + TUD_VENDOR_DESC_LEN)

static_assert(USB_MSG_BLOCK_SIZE == CFG_TUD_VENDOR_EPSIZE,
    "a block of the file link has to be one bulk packet");

uint8_t const desc_configuration[] = {
// Config number, interface count, string index, total length, attributes,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, boot protocol, report descriptor length,
    // EP OUT address, EP IN address, EP size, polling interval (1 ms = 1000 Hz)
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
                             FFB_HID_REPORT_DESC_LEN, EPNUM_HID_OUT, EPNUM_HID_IN,
                             CFG_TUD_HID_EP_BUFSIZE, 1),

    // Interface number, string index, EP OUT address, EP IN address, EP size.
    // Vendor class 0xFF: no HID, so the input APIs of the PC never see it
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_FILES, STRID_FILES, EPNUM_FILES_OUT,
                          EPNUM_FILES_IN, CFG_TUD_VENDOR_EPSIZE)};

uint8_t const*
tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return desc_configuration;
}

/* MICROSOFT OS 2.0 */

// Windows binds its WinUSB driver to the file link by itself, with no INF: the
// BOS points to a descriptor set, which the host then asks for with a vendor
// request. Windows 8.1 and later. Linux and macOS need none of this: libusb
// opens the interface as it is
#define VENDOR_REQUEST_MS_OS_20 0x01 // bRequest of that request, our choice
#define MS_OS_20_DESCRIPTOR_INDEX 7 // its wIndex, fixed by Microsoft
#define MS_OS_20_DESC_LEN 0xB2
#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

// Asked for by the host because bcdUSB is 2.1
static uint8_t const desc_bos[] = {
// Total length, number of device capabilities
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    // Length of the descriptor set, vendor code of its request
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MS_OS_20)};
static_assert(sizeof(desc_bos) == BOS_TOTAL_LEN, "desc_bos is not BOS_TOTAL_LEN");

uint8_t const*
tud_descriptor_bos_cb(void) {
  return desc_bos;
}

// One function: interface 1 is compatible with WINUSB. Its interface GUID,
// {722FD992-DC0C-4973-BBCB-B87CE51AA9B3}, is how the WinUSB API finds the
// interface, and libusb needs it to open it. It never changes: the
// applications of the PC know it
// @formatter:off
static uint8_t const desc_ms_os_20[] = {
    // Set header: length, type, Windows version (8.1), total length
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    // Configuration subset: length, type, configuration index, reserved,
    // length up to the end of the set
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),

    // Function subset: length, type, first interface, reserved, length up to
    // the end of the set
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    ITF_NUM_FILES, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),

    // Compatible ID: length, type, compatible ID, sub-compatible ID
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Registry property: length, type, REG_MULTI_SZ, length of the name, then
    // the name "DeviceInterfaceGUIDs" in UTF-16 with its zero
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A),
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0, 't', 0,
    'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0,
    'D', 0, 's', 0, 0, 0,
    // Length of the data, then the GUID in UTF-16 and the two zeros that end
    // a REG_MULTI_SZ
    U16_TO_U8S_LE(0x0050),
    '{', 0, '7', 0, '2', 0, '2', 0, 'F', 0, 'D', 0, '9', 0, '9', 0, '2', 0,
    '-', 0, 'D', 0, 'C', 0, '0', 0, 'C', 0, '-', 0, '4', 0, '9', 0, '7', 0,
    '3', 0, '-', 0, 'B', 0, 'B', 0, 'C', 0, 'B', 0, '-', 0, 'B', 0, '8', 0,
    '7', 0, 'C', 0, 'E', 0, '5', 0, '1', 0, 'A', 0, 'A', 0, '9', 0, 'B', 0,
    '3', 0, '}', 0, 0, 0, 0, 0
};
// @formatter:on
static_assert(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN,
    "desc_ms_os_20 is not MS_OS_20_DESC_LEN");

// Every vendor request of the host comes here. The one of the BOS gets the
// descriptor set, anything else is stalled. Only a request from the device to
// the host is answered: the set is in flash, nothing may be written into it
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request) {
  // Nothing to do once the data is sent
  if (stage != CONTROL_STAGE_SETUP) {
    return true;
  }
  if (request->bmRequestType_bit.direction == TUSB_DIR_IN && request->bRequest
      == VENDOR_REQUEST_MS_OS_20
      && request->wIndex == MS_OS_20_DESCRIPTOR_INDEX) {
    // At most wLength bytes leave
    return tud_control_xfer(rhport, request, (void*)(uintptr_t)desc_ms_os_20,
                            sizeof(desc_ms_os_20));
  }
  return false;
}

/* STRING DESCRIPTORS */

// @formatter:off
static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: language, English (0x0409)
    "Open FFBoard",             // 1: manufacturer
    "FFB Wheel",                // 2: product
    "000001",                   // 3: serial, ideally from the chip id
    "FFB Wheel file link",      // 4: STRID_FILES, the interface of usb_msg.h
};
// @formatter:on
static uint16_t _desc_str[32];

// Builds the UTF-16 string descriptor in _desc_str
uint16_t const*
tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
      return NULL;
    }
    const char *str = string_desc_arr[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31) chr_count = 31;
    for (uint8_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
  }

  // Header: the length in bytes (header included), then the descriptor type
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}
