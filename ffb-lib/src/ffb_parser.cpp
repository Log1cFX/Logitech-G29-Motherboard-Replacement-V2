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
 * ffb_parser.cpp
 *
 * Decodes the host-issued HID PID reports. Logic ported verbatim from
 * OpenFFBoard HidFFB.cpp. Differences:
 *
 *   - effects array is borrowed from the supplied Calculator instead of
 *     a std::shared_ptr<EffectsCalculator>.
 *   - HID_SendReport() (tud_hid_report) replaced by an optional callback.
 *   - logSerialDebug() calls replaced by FFB_LOG().
 *   - fxUpdateEvent() / cfUpdateEvent() telemetry removed.
 */

#include "ffb/ffb_parser.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
#  define M_PI 3.14159265358979323846f
#endif

namespace {
template <typename T>
inline T clip_t(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
} /* anonymous namespace */

namespace ffb {

HidParser::HidParser(Calculator& c, uint8_t axes)
    : calc(c), axis_count(axes)
{
    /* Direction enable bit lives immediately after the axis enable bits
     * (one bit per axis). Matches OpenFFBoard FFBWheel / FFBJoystick. */
    directionEnableMask = static_cast<uint8_t>(1u << axis_count);

    blockLoad_report.effectBlockIndex = 1;
    blockLoad_report.ramPoolAvailable =
        static_cast<uint16_t>((FFB_MAX_EFFECTS - used_effects) * sizeof(Effect));
    blockLoad_report.loadStatus = 1;

    pool_report.ramPoolSize            = FFB_MAX_EFFECTS * sizeof(Effect);
    pool_report.maxSimultaneousEffects = FFB_MAX_EFFECTS;
    pool_report.memoryManagement       = 1;
}

void HidParser::setActive(bool on) {
    ffb_active = on;
    calc.setActive(on);
}

void HidParser::setGain(uint8_t g) {
    calc.setGlobalGain(g);
}

void HidParser::resetAll() {
    for (uint8_t i = 0; i < FFB_MAX_EFFECTS; ++i) {
        calc.freeEffect(i);
    }
    reportFFBStatus.status = HID_ACTUATOR_POWER | HID_ENABLE_ACTUATORS;
    used_effects = 1;
}

void HidParser::sendStatusReport() {
    reportFFBStatus.status = HID_ACTUATOR_POWER;
    if (ffb_active) {
        reportFFBStatus.status |= HID_ENABLE_ACTUATORS;
        reportFFBStatus.status |= HID_EFFECT_PLAYING;
    } else {
        reportFFBStatus.status |= HID_EFFECT_PAUSE;
    }
    if (send_report_cb) {
        send_report_cb(reinterpret_cast<const uint8_t*>(&reportFFBStatus),
                       sizeof(reportFFBStatus));
    }
}

/* ----------- USB callback entry points ----------------------------- */

void HidParser::hidOut(uint8_t report_id, const uint8_t* buffer, uint16_t bufsize) {
    if (buffer == nullptr || bufsize == 0) return;

    uint8_t event_idx = report_id - FFB_ID_OFFSET;

    switch (event_idx) {
    case HID_ID_NEWEFREP:
        newEffect(reinterpret_cast<const FFB_CreateNewEffect_Feature_Data_t*>(buffer));
        break;

    case HID_ID_EFFREP: {
        /* Initialise to in-class defaults then overwrite with the host's
         * bytes. Some hosts send a shorter report when fewer axes are
         * advertised; the unfilled tail keeps its default values. */
        FFB_SetEffect_t tmp{};
        std::memcpy(&tmp, buffer,
                    std::min<uint16_t>(sizeof(FFB_SetEffect_t), bufsize));
        setEffect(&tmp);
        break;
    }

    case HID_ID_CTRLREP:
        if (bufsize >= 2) {
            controlCmd(buffer[1]);
        }
        break;

    case HID_ID_GAINREP:
        if (bufsize >= 2) {
            setGain(buffer[1]);
        }
        break;

    case HID_ID_ENVREP:
        setEnvelope(reinterpret_cast<const FFB_SetEnvelope_Data_t*>(buffer));
        break;

    case HID_ID_CONDREP:
        setCondition(reinterpret_cast<const FFB_SetCondition_Data_t*>(buffer));
        break;

    case HID_ID_PRIDREP:
        setPeriodic(reinterpret_cast<const FFB_SetPeriodic_Data_t*>(buffer));
        break;

    case HID_ID_CONSTREP:
        setConstantForce(reinterpret_cast<const FFB_SetConstantForce_Data_t*>(buffer));
        break;

    case HID_ID_RAMPREP:
        setRamp(reinterpret_cast<const FFB_SetRamp_Data_t*>(buffer));
        break;

    case HID_ID_EFOPREP:
        setEffectOperation(reinterpret_cast<const FFB_EffOp_Data_t*>(buffer));
        break;

    case HID_ID_BLKFRREP:
        if (bufsize >= 2) {
            calc.freeEffect(buffer[1] - 1);
        }
        break;

    case HID_ID_CSTMREP:
    case HID_ID_SMPLREP:
    default:
        /* Custom force and download-sample are intentionally unsupported,
         * matching the original firmware. */
        break;
    }
}

uint16_t HidParser::hidGet(uint8_t report_id, uint8_t* buffer, uint16_t /*reqlen*/) {
    if (buffer == nullptr) return 0;

    uint8_t id = report_id - FFB_ID_OFFSET;

    switch (id) {
    case HID_ID_BLKLDREP:
        std::memcpy(buffer, &blockLoad_report, sizeof(blockLoad_report));
        return sizeof(blockLoad_report);
    case HID_ID_POOLREP:
        std::memcpy(buffer, &pool_report, sizeof(pool_report));
        return sizeof(pool_report);
    default:
        return 0;
    }
}

/* ----------- Individual report handlers ---------------------------- */

void HidParser::newEffect(const FFB_CreateNewEffect_Feature_Data_t* in_effect) {
    int32_t index = calc.findFreeEffect(in_effect->effectType);
    if (index == -1) {
        blockLoad_report.loadStatus = 2;
        FFB_LOG("FFB: cannot allocate new effect\n");
        return;
    }
    Effect new_effect;
    new_effect.type = in_effect->effectType;
    calc.setFilters(&new_effect);

    calc.effects[index] = std::move(new_effect);

    blockLoad_report.effectBlockIndex = static_cast<uint8_t>(index + 1);
    used_effects++;
    blockLoad_report.ramPoolAvailable =
        static_cast<uint16_t>((FFB_MAX_EFFECTS - used_effects) * sizeof(Effect));
    blockLoad_report.loadStatus = 1;
    sendStatusReport();
}

void HidParser::controlCmd(uint8_t cmd) {
    /* Bitfield from Device Control report:
       0x01 enable, 0x02 disable, 0x04 stop, 0x08 reset,
       0x10 pause, 0x20 continue. */
    if (cmd & 0x01) setActive(true);
    if (cmd & 0x02) setActive(false);
    if (cmd & 0x04) setActive(false);
    if (cmd & 0x08) { setActive(false); resetAll(); }
    if (cmd & 0x10) setActive(false);
    if (cmd & 0x20) setActive(true);
}

void HidParser::setEffect(const FFB_SetEffect_t* effect) {
    uint8_t index = effect->effectBlockIndex;
    if (index == 0 || index > FFB_MAX_EFFECTS) return;

    Effect* p = &calc.effects[index - 1];

    if (p->type != effect->effectType) {
        p->startTime = 0;
        p->type = effect->effectType;
        calc.setFilters(p);
    }

    p->gain         = effect->gain;
    p->type         = effect->effectType;
    p->samplePeriod = effect->samplePeriod;

    bool directionEnable    = (effect->enableAxis & directionEnableMask);
    bool overridesCondition = false;

    /* Conditional effects with multi-axis conditions override direction. */
    if (!p->useSingleCondition &&
        (effect->effectType == FFB_EFFECT_SPRING  ||
         effect->effectType == FFB_EFFECT_DAMPER  ||
         effect->effectType == FFB_EFFECT_INERTIA ||
         effect->effectType == FFB_EFFECT_FRICTION))
    {
        if (p->conditions[0].isActive()) {
            p->axisMagnitudes[0] = 1.0f;
            overridesCondition = true;
        }
        if (FFB_MAX_AXIS >= 2 &&
            (p->conditions[1 % FFB_MAX_AXIS].isActive() || p->useSingleCondition))
        {
            p->axisMagnitudes[1 % FFB_MAX_AXIS] = 1.0f;
            overridesCondition = true;
        }
    }

    if (!overridesCondition) {
        float phaseX = static_cast<float>(M_PI) * 2.0f *
                       (effect->directionX / 36000.0f);

        p->axisMagnitudes[0] = directionEnable
            ? std::sin(phaseX)
            : ((effect->enableAxis & 0x01)
                ? (effect->directionX - 18000.0f) / 18000.0f
                : 0.0f);

#if FFB_MAX_AXIS >= 2
        p->axisMagnitudes[1] = directionEnable
            ? -std::cos(phaseX)
            : ((effect->enableAxis & 0x02)
                ? -(effect->directionY - 18000.0f) / 18000.0f
                : 0.0f);
#endif
    }

#if FFB_MAX_AXIS >= 3
    {
        float phaseY = static_cast<float>(M_PI) * 2.0f *
                       (effect->directionY / 36000.0f);
        /* The original code wrote axisMagnitudes[3] (a bug in the source)
         * when MAX_AXIS == 3; preserve the spirit (third axis vector)
         * by writing index 2. */
        p->axisMagnitudes[2] = directionEnable ? std::sin(phaseY) : 0.0f;
    }
#endif

    if (effect->duration == 0) {
        p->duration = FFB_EFFECT_DURATION_INFINITE;
    } else {
        p->duration = effect->duration;
    }
    p->startDelay = effect->startDelay;

    if (!ffb_active) setActive(true);
    sendStatusReport();
}

void HidParser::setCondition(const FFB_SetCondition_Data_t* cond) {
    if (cond->effectBlockIndex == 0 || cond->effectBlockIndex > FFB_MAX_EFFECTS) {
        return;
    }
    uint8_t axis = cond->parameterBlockOffset;
    if (axis >= axis_count) axis = axis_count - 1;
    if (axis >= FFB_MAX_AXIS) return;

    Effect* p = &calc.effects[cond->effectBlockIndex - 1];
    p->conditions[axis].cpOffset            = cond->cpOffset;
    p->conditions[axis].negativeCoefficient = cond->negativeCoefficient;
    p->conditions[axis].positiveCoefficient = cond->positiveCoefficient;
    p->conditions[axis].negativeSaturation  = cond->negativeSaturation;
    p->conditions[axis].positiveSaturation  = cond->positiveSaturation;
    p->conditions[axis].deadBand            = cond->deadBand;

    if (axis > 0 && axis < FFB_MAX_AXIS && p->conditions[axis].isActive()) {
        p->useSingleCondition = false;
    }
    if ((p->conditions[axis].isActive() ||
         (axis > 0 && p->useSingleCondition)) &&
         p->axisMagnitudes[axis] == 0)
    {
        p->axisMagnitudes[axis] = 1.0f;
    }
}

void HidParser::setEffectOperation(const FFB_EffOp_Data_t* report) {
    if (report->effectBlockIndex == 0 || report->effectBlockIndex > FFB_MAX_EFFECTS) {
        return;
    }
    uint8_t id = report->effectBlockIndex - 1;
    Effect& e  = calc.effects[id];

    if (report->state == 3) {
        e.state = 0;
        FFB_LOG("FFB: stop effect, at index %d\n", report->effectBlockIndex);
    } else {
        if (report->state == 2) {
            FFB_LOG("FFB: start solo, at index %d\n", report->effectBlockIndex);
            for (Effect& other : calc.effects) {
                other.state = 0;
            }
        }
        if (e.state != 1) {
            calc.setFilters(&e);
        }
        FFB_LOG("FFB: start effect, at index %d\n", report->effectBlockIndex);
        e.startTime = calc.millisNow() + e.startDelay;
        e.state     = 1;
    }
}

void HidParser::setEnvelope(const FFB_SetEnvelope_Data_t* report) {
    if (report->effectBlockIndex == 0 || report->effectBlockIndex > FFB_MAX_EFFECTS) {
        return;
    }
    Effect* p = &calc.effects[report->effectBlockIndex - 1];
    p->attackLevel = report->attackLevel;
    p->attackTime  = report->attackTime;
    p->fadeLevel   = report->fadeLevel;
    p->fadeTime    = report->fadeTime;
    p->useEnvelope = true;
}

void HidParser::setRamp(const FFB_SetRamp_Data_t* report) {
    if (report->effectBlockIndex == 0 || report->effectBlockIndex > FFB_MAX_EFFECTS) {
        return;
    }
    Effect* p = &calc.effects[report->effectBlockIndex - 1];
    p->magnitude  = 0x7fff;  /* envelope assumes full magnitude */
    p->startLevel = report->startLevel;
    p->endLevel   = report->endLevel;
}

void HidParser::setConstantForce(const FFB_SetConstantForce_Data_t* report) {
    if (report->effectBlockIndex == 0 || report->effectBlockIndex > FFB_MAX_EFFECTS) {
        return;
    }
    Effect& e = calc.effects[report->effectBlockIndex - 1];
    e.magnitude = report->magnitude;
}

void HidParser::setPeriodic(const FFB_SetPeriodic_Data_t* report) {
    if (report->effectBlockIndex == 0 || report->effectBlockIndex > FFB_MAX_EFFECTS) {
        return;
    }
    Effect* p = &calc.effects[report->effectBlockIndex - 1];
    p->period    = static_cast<uint16_t>(clip_t<uint32_t>(report->period, 1, 0x7fff));
    p->magnitude = report->magnitude;
    p->offset    = report->offset;
    p->phase     = report->phase;
}

} /* namespace ffb */
