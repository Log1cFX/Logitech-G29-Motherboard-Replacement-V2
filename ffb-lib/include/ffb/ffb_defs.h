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
 * ffb_defs.h
 *
 * USB HID PID report layouts, IDs, and effect-type constants.
 * Ported from OpenFFBoard ffb_defs.h. The wire-format structures and
 * numeric constants are bit-for-bit identical to the original; the
 * differences are:
 *   - dependencies on cppmain.h / constants.h removed
 *   - HID_GamepadReport<> templates removed (this library only owns the
 *     FFB pipeline; users build their own input report)
 *   - FFB_Effect struct moved to ffb_effect.h (depends on Biquad)
 *
 * All wire structures keep their __attribute__((packed)) layout so the
 * USB host's bytes can be cast directly into them.
 */

#ifndef FFB_DEFS_H_
#define FFB_DEFS_H_

#include <cstdint>

#include "ffb/ffb_config.h"

#ifndef FFB_PACKED
#  if defined(__GNUC__)
#    define FFB_PACKED __attribute__((packed))
#  else
#    define FFB_PACKED
#  endif
#endif

namespace ffb {

/* --------- HID descriptor usage codes (Physical Interface, effects) --- */
constexpr uint8_t HID_USAGE_CONST = 0x26;
constexpr uint8_t HID_USAGE_RAMP  = 0x27;
constexpr uint8_t HID_USAGE_SQUR  = 0x30;
constexpr uint8_t HID_USAGE_SINE  = 0x31;
constexpr uint8_t HID_USAGE_TRNG  = 0x32;
constexpr uint8_t HID_USAGE_STUP  = 0x33;
constexpr uint8_t HID_USAGE_STDN  = 0x34;
constexpr uint8_t HID_USAGE_SPRNG = 0x40;
constexpr uint8_t HID_USAGE_DMPR  = 0x41;
constexpr uint8_t HID_USAGE_INRT  = 0x42;
constexpr uint8_t HID_USAGE_FRIC  = 0x43;

/* --------- Report IDs (host writes / device replies) ----------------- */
constexpr uint8_t HID_ID_STATE    = 0x02;  /* IN  PID State                  */
constexpr uint8_t HID_ID_EFFREP   = 0x01;  /* OUT Set Effect Report          */
constexpr uint8_t HID_ID_ENVREP   = 0x02;  /* OUT Set Envelope               */
constexpr uint8_t HID_ID_CONDREP  = 0x03;  /* OUT Set Condition              */
constexpr uint8_t HID_ID_PRIDREP  = 0x04;  /* OUT Set Periodic               */
constexpr uint8_t HID_ID_CONSTREP = 0x05;  /* OUT Set Constant Force         */
constexpr uint8_t HID_ID_RAMPREP  = 0x06;  /* OUT Set Ramp Force             */
constexpr uint8_t HID_ID_CSTMREP  = 0x07;  /* OUT Custom Force Data          */
constexpr uint8_t HID_ID_SMPLREP  = 0x08;  /* OUT Download Force Sample      */
constexpr uint8_t HID_ID_EFOPREP  = 0x0A;  /* OUT Effect Operation           */
constexpr uint8_t HID_ID_BLKFRREP = 0x0B;  /* OUT PID Block Free             */
constexpr uint8_t HID_ID_CTRLREP  = 0x0C;  /* OUT PID Device Control         */
constexpr uint8_t HID_ID_GAINREP  = 0x0D;  /* OUT Device Gain                */
constexpr uint8_t HID_ID_SETCREP  = 0x0E;  /* OUT Set Custom Force           */
constexpr uint8_t HID_ID_NEWEFREP = 0x11;  /* FEATURE Create New Effect      */
constexpr uint8_t HID_ID_BLKLDREP = 0x12;  /* FEATURE Block Load reply       */
constexpr uint8_t HID_ID_POOLREP  = 0x13;  /* FEATURE PID Pool reply         */

/* --------- Effect type codes (FFB_Effect::type, also wire-side) ------ */
constexpr uint8_t FFB_EFFECT_NONE         = 0x00;
constexpr uint8_t FFB_EFFECT_CONSTANT     = 0x01;
constexpr uint8_t FFB_EFFECT_RAMP         = 0x02;
constexpr uint8_t FFB_EFFECT_SQUARE       = 0x03;
constexpr uint8_t FFB_EFFECT_SINE         = 0x04;
constexpr uint8_t FFB_EFFECT_TRIANGLE     = 0x05;
constexpr uint8_t FFB_EFFECT_SAWTOOTHUP   = 0x06;
constexpr uint8_t FFB_EFFECT_SAWTOOTHDOWN = 0x07;
constexpr uint8_t FFB_EFFECT_SPRING       = 0x08;
constexpr uint8_t FFB_EFFECT_DAMPER       = 0x09;
constexpr uint8_t FFB_EFFECT_INERTIA      = 0x0A;
constexpr uint8_t FFB_EFFECT_FRICTION     = 0x0B;
constexpr uint8_t FFB_EFFECT_CUSTOM       = 0x0C;

/* --------- PID State report flags ------------------------------------ */
constexpr uint8_t HID_ACTUATOR_POWER    = 0x08;
constexpr uint8_t HID_SAFETY_SWITCH     = 0x04;
constexpr uint8_t HID_ENABLE_ACTUATORS  = 0x02;
constexpr uint8_t HID_EFFECT_PAUSE      = 0x01;
constexpr uint8_t HID_EFFECT_PLAYING    = 0x10;

constexpr uint16_t FFB_EFFECT_DURATION_INFINITE = 0xFFFF;

/* --------- Wire reports ---------------------------------------------- */

/* PID State input report (device -> host) */
struct FFB_PACKED reportFFB_status_t {
    uint8_t reportId = HID_ID_STATE + FFB_ID_OFFSET;
    /* bit0=Paused, bit1=Actuators Enabled, bit2=Safety Switch,
       bit3=Actuator Power, bit4=Effect Playing */
    uint8_t status   = HID_ACTUATOR_POWER | HID_ENABLE_ACTUATORS;
};

/* Set Effect (host -> device, OUT, ID 1) */
struct FFB_PACKED FFB_SetEffect_t {
    uint8_t  reportId             = HID_ID_EFFREP + FFB_ID_OFFSET;
    uint8_t  effectBlockIndex     = 0;  /* 1..FFB_MAX_EFFECTS  */
    uint8_t  effectType           = 0;
    uint16_t duration             = 0;  /* 0..32767 ms         */
    uint16_t triggerRepeatInterval= 0;  /* 0..32767 ms         */
    uint16_t samplePeriod         = 0;  /* 0..32767 ms         */
    uint16_t startDelay           = 0;  /* 0..32767 ms         */
    uint8_t  gain                 = 255;
    uint8_t  triggerButton        = 0;
    uint8_t  enableAxis           = 0;  /* bits: 0=X, 1=Y, 2=DirectionEnable */
    uint16_t directionX           = 0;  /* 0..36000 (deg*100)  */
    uint16_t directionY           = 0;
};

/* Set Condition (host -> device, OUT, ID 3) */
struct FFB_PACKED FFB_SetCondition_Data_t {
    uint8_t  reportId             = HID_ID_CONDREP + FFB_ID_OFFSET;
    uint8_t  effectBlockIndex     = 0;
    uint8_t  parameterBlockOffset = 0;  /* low 2-6 bits index axis */
    int16_t  cpOffset             = 0;
    int16_t  positiveCoefficient  = 0;
    int16_t  negativeCoefficient  = 0;
    uint16_t positiveSaturation   = 0;
    uint16_t negativeSaturation   = 0;
    uint16_t deadBand             = 0;
};

/* Create New Effect (Feature SET, control EP, ID 0x11) */
struct FFB_PACKED FFB_CreateNewEffect_Feature_Data_t {
    uint8_t  effectType = 0;
    uint16_t byteCount  = 0;
};

/* Block Load reply (Feature GET, control EP, ID 0x12) */
struct FFB_PACKED FFB_BlockLoad_Feature_Data_t {
    uint8_t  effectBlockIndex = 0;
    uint8_t  loadStatus       = 1;     /* 1=Success, 2=Full, 3=Error */
    uint16_t ramPoolAvailable = 0;
};

/* PID Pool reply (Feature GET, control EP, ID 0x13) */
struct FFB_PACKED FFB_PIDPool_Feature_Data_t {
    uint16_t ramPoolSize           = FFB_MAX_EFFECTS;
    uint8_t  maxSimultaneousEffects= FFB_MAX_EFFECTS;
    uint8_t  memoryManagement      = 1;
};

/* Set Periodic (host -> device, OUT, ID 4) */
struct FFB_PACKED FFB_SetPeriodic_Data_t {
    uint8_t  reportId         = HID_ID_PRIDREP + FFB_ID_OFFSET;
    uint8_t  effectBlockIndex = 0;
    uint16_t magnitude        = 0;
    int16_t  offset           = 0;
    uint16_t phase            = 0;   /* in degrees*100, 0..35999 */
    uint32_t period           = 0;   /* in ms                    */
};

/* Set Envelope (host -> device, OUT, ID 2) */
struct FFB_PACKED FFB_SetEnvelope_Data_t {
    uint8_t  reportId         = HID_ID_ENVREP + FFB_ID_OFFSET;
    uint8_t  effectBlockIndex = 0;
    uint16_t attackLevel      = 0;
    uint16_t fadeLevel        = 0;
    uint32_t attackTime       = 0;
    uint32_t fadeTime         = 0;
};

/* Set Ramp (host -> device, OUT, ID 6) */
struct FFB_PACKED FFB_SetRamp_Data_t {
    uint8_t  reportId         = HID_ID_RAMPREP + FFB_ID_OFFSET;
    uint8_t  effectBlockIndex = 0;
    uint16_t startLevel       = 0;
    uint16_t endLevel         = 0;
};

/* Effect Operation start/stop (host -> device, OUT, ID 0x0A) */
struct FFB_PACKED FFB_EffOp_Data_t {
    uint8_t reportId         = HID_ID_EFOPREP + FFB_ID_OFFSET;
    uint8_t effectBlockIndex = 0;
    uint8_t state            = 0;  /* 1=start, 2=start solo, 3=stop */
    uint8_t loopCount        = 0;
};

/* Set Constant Force (host -> device, OUT, ID 5). This is the hot path: it is
 * the report streamed most frequently while a constant force effect is active. */
struct FFB_PACKED FFB_SetConstantForce_Data_t {
    uint8_t  reportId         = HID_ID_CONSTREP + FFB_ID_OFFSET;
    uint8_t  effectBlockIndex = 0;
    int16_t  magnitude        = 0;
};

/* Per-axis condition parameter block (used inside Effect, not on the wire). */
struct FFB_Effect_Condition {
    int16_t  cpOffset            = 0;
    int16_t  positiveCoefficient = 0;
    int16_t  negativeCoefficient = 0;
    uint16_t positiveSaturation  = 0;
    uint16_t negativeSaturation  = 0;
    uint16_t deadBand            = 0;

    bool isActive() const {
        return (positiveCoefficient != 0 && positiveSaturation != 0) ||
               (negativeCoefficient != 0 && negativeSaturation != 0);
    }
};

} /* namespace ffb */

#endif /* FFB_DEFS_H_ */
