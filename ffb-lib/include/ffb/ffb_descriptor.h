/*
 * SPDX-License-Identifier: MIT
 *
 * MIT License
 *
 * Copyright (c) 2020 Yannick Richter (OpenFFBoard project and contributors)
 * Copyright (c) 2026 Santryan Raffi
 *
 * Derived from OpenFFBoard (https://github.com/Ultrawipf/OpenFFBoard).
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * ffb_descriptor.h
 *
 * USB HID Report Descriptor for the FFB device.
 *
 * Two consumer paths:
 *
 *   1. Easy path - pre-built byte arrays:
 *
 *        const uint8_t* desc = ffb::descriptor1Axis(&len);
 *
 *      Hand `desc` (length `len`) straight to your USB stack's
 *      "Get HID Report Descriptor" callback.
 *
 *   2. Advanced path - macros for custom assembly. The HIDDESC_FFB_*
 *      building blocks below are byte-for-byte the same as the macros
 *      OpenFFBoard ships in usb_hid_ffb_desc.h; the only difference is
 *      the file is self-contained (no tinyusb / cppmain.h includes).
 */

#ifndef FFB_DESCRIPTOR_H_
#define FFB_DESCRIPTOR_H_

#include <cstdint>

#include "ffb/ffb_config.h"
#include "ffb/ffb_defs.h"

namespace ffb {

/* ---- Pre-built descriptors (resolved at compile time) ---- */

/* 1-axis, 16-bit axis resolution. Matches OpenFFBoard hid_1ffb_desc[]
 * when compiled with the default settings. */
const uint8_t* descriptor1Axis(uint16_t* out_len);

/* 2-axis, 16-bit axis resolution. Matches OpenFFBoard hid_2ffb_desc[]. */
const uint8_t* descriptor2Axis(uint16_t* out_len);

} /* namespace ffb */


/* ====================================================================
 *  Building-block macros (FFB descriptor segments)
 * ====================================================================
 *
 * If you only need the pre-built arrays above, ignore everything below.
 * The macros below let you compose a custom descriptor (different
 * gamepad layout, extra report IDs, etc).
 */

/* HID spec usage IDs we need (Generic Desktop page = 0x01). */
#define FFB_HID_USAGE_DESKTOP_X       0x30
#define FFB_HID_USAGE_DESKTOP_Y       0x31
#define FFB_HID_USAGE_DESKTOP_Z       0x32
#define FFB_HID_USAGE_DESKTOP_RX      0x33
#define FFB_HID_USAGE_DESKTOP_RY      0x34
#define FFB_HID_USAGE_DESKTOP_RZ      0x35
#define FFB_HID_USAGE_DESKTOP_DIAL    0x37
#define FFB_HID_USAGE_DESKTOP_SLIDER  0x36

/* INPUT/OUTPUT/FEATURE Main items (1-byte data form). */
#define FFB_HID_INPUT(x)    0x81, (x)
#define FFB_HID_OUTPUT(x)   0x91, (x)

#define FFB_MAX_EFFECTS_BYTE ((uint8_t)FFB_MAX_EFFECTS)

/* --- HID command vendor channel (one IN + one OUT report) ---------- */
#define FFB_HID_CMD_ID 0xA1

#define HIDDESC_CTRL_REP_INPUT \
    0x85, FFB_HID_CMD_ID,             /* Report ID                 */ \
    0x09, 0x01,                       /* USAGE (Vendor)            */ \
    0x15, 0x00,                       /* LOGICAL_MIN 0             */ \
    0x26, 0x04, 0x00,                 /* LOGICAL_MAX 4             */ \
    0x75, 0x08, 0x95, 0x01, FFB_HID_INPUT(1), \
    0x09, 0x02, 0x75, 0x10, 0x95, 0x01, FFB_HID_INPUT(1), \
    0x09, 0x03, 0x75, 0x08, 0x95, 0x01, FFB_HID_INPUT(1), \
    0x09, 0x04, 0x75, 0x20, 0x95, 0x01, FFB_HID_INPUT(1), \
    0x09, 0x05, 0x75, 0x40, 0x95, 0x01, FFB_HID_INPUT(1), \
    0x09, 0x06, 0x75, 0x40, 0x95, 0x01, FFB_HID_INPUT(1)

#define HIDDESC_CTRL_REP_OUTPUT \
    0x85, FFB_HID_CMD_ID, \
    0x09, 0x01, 0x15, 0x00, 0x26, 0x04, 0x00, \
    0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(1), \
    0x09, 0x02, 0x75, 0x10, 0x95, 0x01, FFB_HID_OUTPUT(1), \
    0x09, 0x03, 0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(1), \
    0x09, 0x04, 0x75, 0x20, 0x95, 0x01, FFB_HID_OUTPUT(1), \
    0x09, 0x05, 0x75, 0x40, 0x95, 0x01, FFB_HID_OUTPUT(1), \
    0x09, 0x06, 0x75, 0x40, 0x95, 0x01, FFB_HID_OUTPUT(1)

#define HIDDESC_CTRL_REPORTS \
    0x06, 0x00, 0xFF,             /* USAGE_PAGE (Vendor)           */ \
    0x09, 0x00,                   /* USAGE (Vendor)                */ \
    0xA1, 0x01,                   /* Collection (Application)      */ \
        HIDDESC_CTRL_REP_OUTPUT,  \
        HIDDESC_CTRL_REP_INPUT,   \
    0xC0                          /* END_COLLECTION                */

/* ============================================================
 * HIDDESC_GAMEPAD_16B  -  Report ID 1
 * Payload: 24 bytes (+1 Report ID byte = 25 total)
 * ------------------------------------------------------------
 * Byte    Size   Field
 *  0..7   64 b   Buttons 1..64   (1 bit each)
 *  8..9   16 b   X      signed  -32767..32767
 * 10..11  16 b   Y      signed  -32767..32767
 * 12..13  16 b   Z      signed  -32767..32767
 * 14..15  16 b   Rx     signed  -32767..32767
 * 16..17  16 b   Ry     signed  -32767..32767
 * 18..19  16 b   Rz     signed  -32767..32767
 * 20..21  16 b   Dial   signed  -32767..32767
 * 22..23  16 b   Slider signed  -32767..32767
 * ============================================================ */
#define HIDDESC_GAMEPAD_16B \
    0xA1, 0x00,                              /* COLLECTION (Physical)    */ \
    0x85, 0x01,                              /* REPORT_ID (1)            */ \
    0x05, 0x09,                              /* USAGE_PAGE (Button)      */ \
    0x19, 0x01, 0x29, 0x40,                  /* USAGE_MIN/MAX 1..64      */ \
    0x15, 0x00, 0x25, 0x01,                  /* LOGICAL_MIN/MAX 0..1     */ \
    0x95, 0x40, 0x75, 0x01, FFB_HID_INPUT(2),/* 64 buttons, 1 bit each   */ \
    0x05, 0x01,                              /* USAGE_PAGE Generic Desk. */ \
    0x09, FFB_HID_USAGE_DESKTOP_X,           \
    0x09, FFB_HID_USAGE_DESKTOP_Y,           \
    0x09, FFB_HID_USAGE_DESKTOP_Z,           \
    0x09, FFB_HID_USAGE_DESKTOP_RX,          \
    0x09, FFB_HID_USAGE_DESKTOP_RY,          \
    0x09, FFB_HID_USAGE_DESKTOP_RZ,          \
    0x09, FFB_HID_USAGE_DESKTOP_DIAL,        \
    0x09, FFB_HID_USAGE_DESKTOP_SLIDER,      \
    0x16, 0x01, 0x80,                        /* LOGICAL_MIN -32767       */ \
    0x26, 0xFF, 0x7F,                        /* LOGICAL_MAX  32767       */ \
    0x75, 0x10, 0x95, 0x08, FFB_HID_INPUT(2),/* 8 axes, 16-bit each      */ \
    0xC0

/* ============================================================
 * HIDDESC_G29_TEMPLATE  -  Report ID 1
 * Payload: 9 bytes (+1 Report ID byte = 10 total)
 * ------------------------------------------------------------
 * Byte   Bits    Field
 *  0     0..3    Hat switch   (0..7, null-state)
 *  0     4..7    Buttons 1..4
 *  1     0..7    Buttons 5..12
 *  2     0..7    Buttons 13..20
 *  3     0       Padding (1 bit, const)
 *  3     1..7    Buttons 21..27  (shifter)
 *  4..5  16 b    X    signed   -32767..32767  (steering)
 *  6      8 b    Z    unsigned 0..255         (clutch)
 *  7      8 b    Rx   unsigned 0..255         (brake)
 *  8      8 b    Ry   unsigned 0..255         (throttle)
 * ============================================================ */
#define HIDDESC_G29_TEMPLATE \
    0xA1, 0x00,                              /* COLLECTION (Physical)    */ \
	0x85, 0x01,                              /* REPORT_ID (1)            */ \
	0x05, 0x01,                              /* USAGE_PAGE Generic Desk. */ \
	0x09, 0x39,								 /* Usage (Hat Switch) 		 */ \
	0x15, 0x00,								 /* Logical Minimum (0)		 */ \
	0x25, 0x07,								 /* Logical Maximum (7)		 */ \
	0x75, 0x04,								 /* Report Size (4 bits)	 */ \
	0x95, 0x01,								 /* Report Count (1) 		 */ \
	0x81, 0x42,								 /* Input (Data,Var,Abs,Null)*/ \
    0x05, 0x09,                              /* USAGE_PAGE (Button)      */ \
    0x19, 0x01, 0x29, 0x14,                  /* USAGE_MIN/MAX 1..20      */ \
    0x15, 0x00, 0x25, 0x01,                  /* LOGICAL_MIN/MAX 0..1     */ \
    0x95, 0x14, 0x75, 0x01, FFB_HID_INPUT(2),/* 20 buttons, 1 bit each   */ \
 	0x95, 0x01,								 /* REPORT_COUNT (1) 		 */ \
 	0x81, 0x03,								 /* 1bit padding			 */ \
 	0x19, 0x15,								 /* USAGE_MINIMUM (Button 21)*/ \
 	0x29, 0x1B,								 /* USAGE_MAXIMUM (Button 27)*/ \
 	0x95, 0x07,								 /* REPORT_COUNT (7) 		 */ \
 	0x81, 0x02,								 /* 7 buttons for shifter 	 */ \
    0x05, 0x01,                              /* USAGE_PAGE Generic Desk. */ \
    0x09, FFB_HID_USAGE_DESKTOP_X,           \
	0x16, 0x01, 0x80,                        /* LOGICAL_MIN -32767       */ \
	0x26, 0xFF, 0x7F,                        /* LOGICAL_MAX  32767       */ \
	0x75, 0x10, 0x95, 0x01, FFB_HID_INPUT(2),/* 1 axe, 16-bit each      */ \
	0x09, FFB_HID_USAGE_DESKTOP_Z,           \
	0x09, FFB_HID_USAGE_DESKTOP_RX,          \
	0x09, FFB_HID_USAGE_DESKTOP_RY,          \
	0x15, 0x00,                              /* LOGICAL_MIN (0)          */ \
	0x26, 0xFF, 0x00,                        /* LOGICAL_MAX (255)        */ \
	0x75, 0x08, 0x95, 0x03, FFB_HID_INPUT(2),/* 3 axes, 8-bit each       */ \
    0xC0


/* --- PID State input report (ID 2) --------------------------------- */
#define HIDDESC_FFB_STATEREP \
    0x05, 0x0F,                                 /* USAGE_PAGE PI         */ \
    0x09, 0x92,                                 /* PID State report      */ \
    0xA1, 0x02,                                 /* Collection Datalink   */ \
        0x85, HID_ID_STATE + FFB_ID_OFFSET,     /* Report ID 2           */ \
        0x09, 0x9F, 0x09, 0xA0, 0x09, 0xA4, 0x09, 0xA6, 0x09, 0x94,        \
        0x15, 0x00, 0x25, 0x01, 0x35, 0x00, 0x45, 0x01,                    \
        0x75, 0x01, 0x95, 0x05, FFB_HID_INPUT(2),                          \
        0x95, 0x03, FFB_HID_INPUT(3),                                      \
    0xC0

/* --- Set Effect output report (ID 1) ------------------------------- */
#define HIDDESC_FFB_SETEFREP \
    0x09, 0x21, 0xA1, 0x02, \
        0x85, HID_ID_EFFREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
        0x09, 0x25, 0xA1, 0x02, \
            0x09, ffb::HID_USAGE_CONST,  0x09, ffb::HID_USAGE_RAMP,  \
            0x09, ffb::HID_USAGE_SQUR,   0x09, ffb::HID_USAGE_SINE,  \
            0x09, ffb::HID_USAGE_TRNG,   0x09, ffb::HID_USAGE_STUP,  \
            0x09, ffb::HID_USAGE_STDN,   0x09, ffb::HID_USAGE_SPRNG, \
            0x09, ffb::HID_USAGE_DMPR,   0x09, ffb::HID_USAGE_INRT,  \
            0x09, ffb::HID_USAGE_FRIC,                              \
            0x25, 0x0B, 0x15, 0x01, 0x35, 0x01, 0x45, 0x0B, \
            0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(0), \
        0xC0, \
        0x09, 0x50, 0x09, 0x54, 0x09, 0x51, 0x09, 0xA7, \
        0x15, 0x00, 0x26, 0xFF, 0x7F, 0x35, 0x00, 0x46, 0xFF, 0x7F, \
        0x66, 0x03, 0x10, 0x55, 0xFD, \
        0x75, 0x10, 0x95, 0x04, FFB_HID_OUTPUT(2), \
        0x55, 0x00, 0x66, 0x00, 0x00, \
        0x09, 0x52, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x35, 0x00, 0x46, 0x10, 0x27, \
        0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(2), \
        0x09, 0x53, 0x15, 0x01, 0x25, 0x08, 0x35, 0x01, 0x45, 0x08, \
        0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(2)

/* --- Set Envelope (ID 2) ------------------------------------------- */
#define HIDDESC_FFB_SETENVREP \
    0x09, 0x5A, 0xA1, 0x02, \
        0x85, HID_ID_ENVREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
        0x09, 0x5B, 0x09, 0x5D, \
        0x16, 0x00, 0x00, 0x26, 0xFF, 0x7F, 0x36, 0x00, 0x00, 0x46, 0xFF, 0x7F, \
        0x75, 0x10, 0x95, 0x02, FFB_HID_OUTPUT(2), \
        0x09, 0x5C, 0x09, 0x5E, \
        0x66, 0x03, 0x10, 0x55, 0xFD, \
        0x27, 0xFF, 0x7F, 0x00, 0x00, 0x47, 0xFF, 0x7F, 0x00, 0x00, \
        0x75, 0x20, FFB_HID_OUTPUT(2), \
        0x45, 0x00, 0x66, 0x00, 0x00, 0x55, 0x00, \
    0xC0

/* --- Set Periodic (ID 4) ------------------------------------------- */
#define HIDDESC_FFB_SETPERIODICREP \
    0x09, 0x6E, 0xA1, 0x02, \
        0x85, HID_ID_PRIDREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
        0x09, 0x70, 0x16, 0x00, 0x00, 0x26, 0xFF, 0x7F, \
        0x36, 0x00, 0x00, 0x26, 0xFF, 0x7F, \
        0x75, 0x10, 0x95, 0x01, FFB_HID_OUTPUT(2), \
        0x09, 0x6F, 0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, \
        0x36, 0x00, 0x80, 0x46, 0xFF, 0x7F, \
        0x95, 0x01, 0x75, 0x10, FFB_HID_OUTPUT(2), \
        0x09, 0x71, 0x66, 0x14, 0x00, 0x55, 0xFE, \
        0x15, 0x00, 0x27, 0x9F, 0x8C, 0x00, 0x00, \
        0x35, 0x00, 0x47, 0x9F, 0x8C, 0x00, 0x00, \
        0x75, 0x10, 0x95, 0x01, FFB_HID_OUTPUT(2), \
        0x09, 0x72, 0x15, 0x01, 0x27, 0xFF, 0x7F, 0x00, 0x00, \
        0x35, 0x01, 0x47, 0xFF, 0x7F, 0x00, 0x00, \
        0x66, 0x03, 0x10, 0x55, 0xFD, \
        0x75, 0x20, 0x95, 0x01, FFB_HID_OUTPUT(2), \
        0x66, 0x00, 0x00, 0x55, 0x00, \
    0xC0

/* --- Set Constant Force (ID 5) ------------------------------------- */
#define HIDDESC_FFB_SETCFREP \
    0x09, 0x73, 0xA1, 0x02, \
        0x85, HID_ID_CONSTREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
        0x09, 0x70, 0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F, \
        0x36, 0x01, 0x80, 0x46, 0xFF, 0x7F, \
        0x75, 0x10, 0x95, 0x01, FFB_HID_OUTPUT(2), \
    0xC0

/* --- Set Ramp (ID 6) ----------------------------------------------- */
#define HIDDESC_FFB_SETRAMPREP \
    0x09, 0x74, 0xA1, 0x02, \
        0x85, HID_ID_RAMPREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
        0x09, 0x75, 0x09, 0x76, \
        0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, \
        0x36, 0x00, 0x80, 0x46, 0xFF, 0x7F, \
        0x75, 0x10, 0x95, 0x02, FFB_HID_OUTPUT(2), \
    0xC0

/* --- Effect Operation (ID 10) -------------------------------------- */
#define HIDDESC_FFB_EFOPREP \
    0x05, 0x0F, 0x09, 0x77, 0xA1, 0x02, \
        0x85, HID_ID_EFOPREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
        0x09, 0x78, 0xA1, 0x02, \
            0x09, 0x79, 0x09, 0x7A, 0x09, 0x7B, \
            0x15, 0x01, 0x25, 0x03, 0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(0), \
        0xC0, \
        0x09, 0x7C, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x35, 0x00, 0x46, 0xFF, 0x00, \
        FFB_HID_OUTPUT(2), \
    0xC0

/* --- PID Block Free (ID 11) ---------------------------------------- */
#define HIDDESC_FFB_BLOCKFREEREP \
    0x09, 0x90, 0xA1, 0x02, \
        0x85, HID_ID_BLKFRREP + FFB_ID_OFFSET, \
        0x09, 0x22, 0x15, 0x01, 0x25, FFB_MAX_EFFECTS_BYTE, \
        0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, 0x75, 0x08, 0x95, 0x01, \
        FFB_HID_OUTPUT(2), \
    0xC0

/* --- PID Device Control (ID 12) + Device Gain (ID 13) -------------- */
#define HIDDESC_FFB_DEVCTRLREP \
    0x09, 0x95, 0xA1, 0x02, \
    0x85, HID_ID_CTRLREP + FFB_ID_OFFSET, \
    0x09, 0x96, 0xA1, 0x02, \
        0x09, 0x97, 0x09, 0x98, 0x09, 0x99, 0x09, 0x9A, 0x09, 0x9B, 0x09, 0x9C, \
        0x15, 0x01, 0x25, 0x06, 0x75, 0x01, 0x95, 0x08, FFB_HID_OUTPUT(2), \
    0xC0, \
    0xC0, \
    0x09, 0x7D, 0xA1, 0x02, \
        0x85, HID_ID_GAINREP + FFB_ID_OFFSET, \
        0x09, 0x7E, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x35, 0x00, 0x46, 0x10, 0x27, \
        0x75, 0x08, 0x95, 0x01, FFB_HID_OUTPUT(2), \
    0xC0

/* --- Create New Effect (Feature SET, ID 0x11) ---------------------- */
#define HIDDESC_FFB_NEWEFREP \
    0x09, 0xAB, 0xA1, 0x02, \
        0x85, HID_ID_NEWEFREP + FFB_ID_OFFSET, \
        0x09, 0x25, 0xA1, 0x02, \
            0x09, ffb::HID_USAGE_CONST,  0x09, ffb::HID_USAGE_RAMP,  \
            0x09, ffb::HID_USAGE_SQUR,   0x09, ffb::HID_USAGE_SINE,  \
            0x09, ffb::HID_USAGE_TRNG,   0x09, ffb::HID_USAGE_STUP,  \
            0x09, ffb::HID_USAGE_STDN,   0x09, ffb::HID_USAGE_SPRNG, \
            0x09, ffb::HID_USAGE_DMPR,   0x09, ffb::HID_USAGE_INRT,  \
            0x09, ffb::HID_USAGE_FRIC,                              \
            0x25, 0x0B, 0x15, 0x01, 0x35, 0x01, 0x45, 0x0B, \
            0x75, 0x08, 0x95, 0x01, 0xB1, 0x00, \
        0xC0, \
        0x05, 0x01, 0x09, 0x3B, \
        0x15, 0x00, 0x26, 0xFF, 0x01, 0x35, 0x00, 0x46, 0xFF, 0x01, \
        0x75, 0x0A, 0x95, 0x01, 0xB1, 0x02, \
        0x75, 0x06, 0xB1, 0x01, \
    0xC0

/* --- Block Load (Feature GET, ID 0x12) ----------------------------- */
#define HIDDESC_FFB_BLOCKLOADREP \
    0x05, 0x0F, 0x09, 0x89, 0xA1, 0x02, \
    0x85, HID_ID_BLKLDREP + FFB_ID_OFFSET, \
    0x09, 0x22, 0x25, FFB_MAX_EFFECTS_BYTE, 0x15, 0x01, \
    0x35, 0x01, 0x45, FFB_MAX_EFFECTS_BYTE, \
    0x75, 0x08, 0x95, 0x01, 0xB1, 0x02, \
    0x09, 0x8B, 0xA1, 0x02, \
        0x09, 0x8C, 0x09, 0x8D, 0x09, 0x8E, \
        0x15, 0x01, 0x25, 0x03, 0x35, 0x01, 0x45, 0x03, \
        0x75, 0x08, 0x95, 0x01, 0xB1, 0x00, \
    0xC0, \
    0x09, 0xAC, 0x15, 0x00, 0x27, 0xFF, 0xFF, 0x00, 0x00, \
    0x35, 0x00, 0x47, 0xFF, 0xFF, 0x00, 0x00, \
    0x75, 0x10, 0x95, 0x01, 0xB1, 0x00, \
    0xC0

/* --- PID Pool (Feature GET, ID 0x13) ------------------------------- */
#define HIDDESC_FFB_POOLREP \
    0x09, 0x7F, 0xA1, 0x02, \
        0x85, HID_ID_POOLREP + FFB_ID_OFFSET, \
        0x09, 0x80, 0x75, 0x10, 0x95, 0x01, \
        0x15, 0x00, 0x35, 0x00, \
        0x27, 0xFF, 0xFF, 0x00, 0x00, 0x47, 0xFF, 0xFF, 0x00, 0x00, \
        0xB1, 0x02, \
        0x09, 0x83, 0x26, 0xFF, 0x00, 0x46, 0xFF, 0x00, \
        0x75, 0x08, 0x95, 0x01, 0xB1, 0x02, \
        0x09, 0xA9, 0x09, 0xAA, 0x75, 0x01, 0x95, 0x02, \
        0x15, 0x00, 0x25, 0x01, 0x35, 0x00, 0x45, 0x01, 0xB1, 0x02, \
        0x75, 0x06, 0x95, 0x01, 0xB1, 0x03, \
    0xC0

#endif /* FFB_DESCRIPTOR_H_ */
