/*
 * SPDX-License-Identifier: MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Santryan Raffi
 *
 * Part of a standalone force-feedback library derived from OpenFFBoard
 * (https://github.com/Ultrawipf/OpenFFBoard).
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
 * ffb.h
 *
 * Public facade for the standalone OpenFFBoard-derived FFB library.
 *
 * Typical usage (bare-metal, no RTOS):
 *
 *   #include "ffb/ffb.h"
 *
 *   static bool send_status_report(const uint8_t* buf, uint16_t len) {
 *       return my_usb_hid_send(buf, len);
 *   }
 *
 *   ffb::Library lib(1, { my_millis, my_micros });
 *   lib.setSendReportCallback(send_status_report);
 *
 *   // Hand the descriptor to your USB stack at enumeration time:
 *   uint16_t desc_len;
 *   const uint8_t* desc = ffb::Library::descriptor1Axis(&desc_len);
 *
 *   // In your USB Set/Get Report callbacks:
 *   void on_hid_set_report(uint8_t id, const uint8_t* buf, uint16_t len) {
 *       lib.hidOut(id, buf, len);
 *   }
 *   uint16_t on_hid_get_report(uint8_t id, uint8_t* reply, uint16_t maxlen) {
 *       return lib.hidGet(id, reply, maxlen);
 *   }
 *
 *   // Once per FFB tick (e.g. 1 kHz):
 *   lib.setAxisState(0, { wheel_pos_int16, wheel_speed_dps, wheel_accel_dpss });
 *   lib.calculate();
 *   int32_t torque = lib.getAxisTorque(0);   // -0x7fff .. 0x7fff
 */

#ifndef FFB_H_
#define FFB_H_

#include <cstdint>

#include "ffb/ffb_calculator.h"
#include "ffb/ffb_config.h"
#include "ffb/ffb_defs.h"
#include "ffb/ffb_descriptor.h"
#include "ffb/ffb_parser.h"

namespace ffb {

/* Header-only facade that owns the two halves of the engine - the HID parser
 * (decodes the host's reports) and the calculator (runs the force math) - and
 * forwards each public call to the right one. This is the only type most
 * integrators ever touch. */
class Library {
public:
    /* axis_count must be <= FFB_MAX_AXIS; ts supplies the millis()/micros()
     * counters the engine needs for effect timing. The parser is wired to a
     * reference of the calculator so the two share one effect pool. Allocates
     * nothing on the heap. */
    Library(uint8_t axis_count, TimeSource ts)
        : calculator(axis_count, ts), parser(calculator, axis_count) {}

    /* ------- USB receive (call from your USB stack callbacks) ------ */
    void hidOut(uint8_t report_id, const uint8_t* buf, uint16_t len) {
        parser.hidOut(report_id, buf, len);
    }
    uint16_t hidGet(uint8_t report_id, uint8_t* reply, uint16_t reqlen) {
        return parser.hidGet(report_id, reply, reqlen);
    }

    /* ------- Per-tick loop ----------------------------------------- */
    void setAxisState(uint8_t axis, const AxisState& s) {
        calculator.setAxisState(axis, s);
    }
    void calculate() {
        calculator.calculate();
    }
    int32_t getAxisTorque(uint8_t axis) const {
        return calculator.getAxisTorque(axis);
    }

    /* ------- Control / settings ------------------------------------ */
    /* Enable/disable, reset and gain are routed through the parser, which
     * mirrors the change into the calculator so the two never disagree; the
     * read-back getters and the samplerate touch the calculator directly. */
    bool isActive() const            { return parser.isActive(); }
    void setActive(bool on)          { parser.setActive(on); }
    void resetAllEffects()           { parser.resetAll(); }
    void setGlobalGain(uint8_t gain) { parser.setGain(gain); }
    uint8_t getGlobalGain() const    { return calculator.getGlobalGain(); }
    void setSamplerate(float hz)     { calculator.setSamplerate(hz); }
    float getSamplerate() const      { return calculator.getSamplerate(); }
    void setDirectionEnableMask(uint8_t m) { parser.setDirectionEnableMask(m); }

    /* ------- Optional hook for status reports back to host --------- */
    void setSendReportCallback(SendReportFn cb) {
        parser.setSendReportCallback(cb);
    }

    /* ------- Pre-built HID descriptors ----------------------------- */
    static const uint8_t* descriptor1Axis(uint16_t* out_len) {
        return ffb::descriptor1Axis(out_len);
    }
    static const uint8_t* descriptor2Axis(uint16_t* out_len) {
        return ffb::descriptor2Axis(out_len);
    }

    /* ------- Direct access (for users who need it) ----------------- */
    Calculator& getCalculator() { return calculator; }
    HidParser&  getParser()     { return parser; }

private:
    Calculator calculator;
    HidParser  parser;
};

} /* namespace ffb */

#endif /* FFB_H_ */
