/*
 * ffb_parser.h
 *
 * Decodes USB HID PID Output/Feature reports from the host into Effect
 * mutations on a shared Calculator's effect array. Ported from
 * OpenFFBoard HidFFB. The wire-format decoding is bit-identical; the
 * differences are:
 *
 *   - No inheritance from UsbHidHandler / EffectsControlItf.
 *   - No globalHidHandler singleton; user calls hidOut()/hidGet() directly
 *     from their USB stack callbacks.
 *   - tud_hid_report() replaced by a user-supplied SendReportFn callback.
 *   - logSerialDebug() replaced by the FFB_LOG macro (no-op by default).
 */

#ifndef FFB_PARSER_H_
#define FFB_PARSER_H_

#include <cstdint>

#include "ffb/ffb_calculator.h"
#include "ffb/ffb_config.h"
#include "ffb/ffb_defs.h"

namespace ffb {

/* Mirrors the TinyUSB hid_report_type_t enum (HID class spec). The parser
 * only needs to distinguish OUTPUT from FEATURE — the bare values match
 * the standard so user-provided values pass through unchanged. */
enum HidReportType : uint8_t {
    HID_REPORT_TYPE_INVALID = 0,
    HID_REPORT_TYPE_INPUT   = 1,
    HID_REPORT_TYPE_OUTPUT  = 2,
    HID_REPORT_TYPE_FEATURE = 3,
};

/* Callback signature for sending input reports back to the host (only
 * used for the PID State report — IN report ID 2). Should return true if
 * the report was accepted by the USB stack. */
using SendReportFn = bool (*)(const uint8_t* report, uint16_t len);

class HidParser {
public:
    /* The parser is given a reference to the Calculator that owns the
     * effects array, plus the number of axes the device exposes. */
    HidParser(Calculator& calc, uint8_t axis_count);

    /* Inbound USB data. Call from your USB stack's Set Report callback
     * (handles both interrupt-OUT and SET_REPORT on the control endpoint).
     * Pass the report ID as the host sent it; the library subtracts
     * FFB_ID_OFFSET internally. */
    void hidOut(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize);

    /* Outbound USB feature reply. Call from your USB stack's Get Report
     * callback. Returns the number of bytes written to reply_buffer. */
    uint16_t hidGet(uint8_t report_id, uint8_t* reply_buffer, uint16_t reqlen);

    /* Optional: register a callback to push PID State reports back to the
     * host when an effect is created/started/stopped. If unset, status
     * reports are simply not transmitted. */
    void setSendReportCallback(SendReportFn cb) { send_report_cb = cb; }

    /* Direction enable bit position inside FFB_SetEffect_t::enableAxis.
     * Defaults to 1 << axis_count (which matches the OpenFFBoard 1-axis
     * descriptor). Override if you ship a custom descriptor that places
     * the bit elsewhere. */
    void setDirectionEnableMask(uint8_t mask) { directionEnableMask = mask; }

    /* Control entry-points the facade may call directly. */
    void setActive(bool on);
    void setGain(uint8_t gain);
    void resetAll();
    bool isActive() const { return ffb_active; }

    /* Send the PID State report (ID 2) describing current FFB state. Used
     * internally when an effect is created/operated on, but exposed so the
     * application can refresh it on demand. */
    void sendStatusReport();

private:
    /* Parser entry points (one per OUT report). Layout exactly matches
     * the original HidFFB::hidOut switch. */
    void newEffect(const FFB_CreateNewEffect_Feature_Data_t* effect);
    void freeEffect(uint16_t id_zero_based);
    void controlCmd(uint8_t cmd);
    void setEffect(const FFB_SetEffect_t* report);
    void setCondition(const FFB_SetCondition_Data_t* cond);
    void setEnvelope(const FFB_SetEnvelope_Data_t* report);
    void setRamp(const FFB_SetRamp_Data_t* report);
    void setConstantForce(const FFB_SetConstantForce_Data_t* report);
    void setPeriodic(const FFB_SetPeriodic_Data_t* report);
    void setEffectOperation(const FFB_EffOp_Data_t* report);

    Calculator& calc;
    uint8_t  axis_count;
    uint8_t  directionEnableMask;
    uint16_t used_effects   = 0;
    bool     ffb_active     = false;
    FFB_BlockLoad_Feature_Data_t blockLoad_report;
    FFB_PIDPool_Feature_Data_t   pool_report;
    reportFFB_status_t           reportFFBStatus;
    SendReportFn send_report_cb = nullptr;
};

} /* namespace ffb */

#endif /* FFB_PARSER_H_ */
