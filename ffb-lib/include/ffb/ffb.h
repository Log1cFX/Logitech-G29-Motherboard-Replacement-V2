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

class Library {
public:
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
