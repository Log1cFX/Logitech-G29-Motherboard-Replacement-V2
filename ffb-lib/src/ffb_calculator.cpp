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
 * ffb_calculator.cpp
 *
 * Force calculation. All effect-math functions are byte-for-byte equivalent
 * to OpenFFBoard EffectsCalculator.cpp. The only structural changes are:
 *
 *   - Axis input/output goes through internal arrays instead of an Axis
 *     vector (so the file has no Axis.h dependency).
 *   - HAL_GetTick() -> time_source.millis()
 *     micros()      -> time_source.micros()
 *   - effect->filter[i] is now an inline Biquad (not unique_ptr); presence
 *     is signalled by effect->filter_active[i].
 *   - setFilters() uses a plain switch instead of std::function lambdas.
 *   - Statistics, monitor thread, LED hook, persistence stripped.
 */

#include "ffb/ffb_calculator.h"

#include <cmath>
#include <cstdlib>

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

constexpr uint8_t EFFECT_STATE_INACTIVE = 0;

} /* anonymous namespace */

namespace ffb {

Calculator::Calculator(uint8_t axis_count_, TimeSource ts)
    : axis_count(axis_count_), time_source(ts)
{}

void Calculator::setSamplerate(float hz) {
    if (hz <= 0) hz = FFB_DEFAULT_SAMPLERATE_HZ;
    this->calcfrequency = hz;
    /* Match EffectsCalculator::updateSamplerate(): rebuild filters on every
     * effect that already has one. */
    for (Effect& e : effects) {
        if (e.filter_active[0]) {
            setFilters(&e);
        }
    }
}

void Calculator::setFrictionRampupPct(uint8_t pct) {
    frictionPctSpeedToRampup = clip_t<uint8_t>(pct, 0, 100);
}

void Calculator::setFilterProfileId(uint8_t id) {
    filterProfileId = clip_t<uint8_t>(id, 0, 1);
    updateFiltersForType(FFB_EFFECT_DAMPER);
    updateFiltersForType(FFB_EFFECT_FRICTION);
    updateFiltersForType(FFB_EFFECT_INERTIA);
}

EffectFilterPreset& Calculator::filterPreset(uint8_t profile) {
    if (profile > 1) profile = 1;
    return filter_presets[profile];
}

void Calculator::setAxisState(uint8_t axis, const AxisState& s) {
    if (axis < FFB_MAX_AXIS) {
        axis_state[axis] = s;
    }
}

int32_t Calculator::getAxisTorque(uint8_t axis) const {
    if (axis < FFB_MAX_AXIS) {
        return axis_torque[axis];
    }
    return 0;
}

int32_t Calculator::findFreeEffect(uint8_t type) {
    if (type > FFB_EFFECT_NONE && type < FFB_EFFECT_CUSTOM + 1) {
        for (uint8_t i = 0; i < effects.size(); ++i) {
            if (effects[i].type == FFB_EFFECT_NONE) {
                return i;
            }
        }
    }
    return -1;
}

void Calculator::freeEffect(uint16_t idx) {
    if (idx < effects.size()) {
        effects[idx] = Effect();  /* Reset all fields */
        for (int i = 0; i < FFB_MAX_AXIS; ++i) {
            effects[idx].filter_active[i] = false;
        }
    }
}

/* ----------- Main per-tick loop ------------------------------------- */

void Calculator::calculate() {
    /* No active state -> zero all torques. */
    for (uint8_t a = 0; a < axis_count; ++a) {
        axis_torque[a] = 0;
    }

    if (!isActive()) {
        return;
    }

    int32_t force = 0;
    int32_t forces[FFB_MAX_AXIS] = {0};

    const uint32_t now_ms = time_source.millis();

    for (uint8_t fxi = 0; fxi < FFB_MAX_EFFECTS; ++fxi) {
        Effect* effect = &effects[fxi];

        /* Active and not infinite (0 or 0xffff)? */
        if (effect->state != EFFECT_STATE_INACTIVE &&
            effect->duration != FFB_EFFECT_DURATION_INFINITE &&
            effect->duration != 0)
        {
            if (now_ms < effect->startTime) {
                continue;                            /* start delay */
            }
            if (now_ms - effect->startTime > effect->duration) {
                effect->state = EFFECT_STATE_INACTIVE;
            }
        }

        if (effect->state == EFFECT_STATE_INACTIVE) {
            continue;
        }

        force = calcNonConditionEffectForce(effect);

        for (uint8_t axis = 0; axis < axis_count; ++axis) {
            int32_t axisforce = calcComponentForce(effect, force, axis);
            forces[axis] += axisforce;
        }
    }

    for (uint8_t a = 0; a < axis_count; ++a) {
        axis_torque[a] = clip_t<int32_t>(forces[a], -0x7fff, 0x7fff);
    }
}

/* ----------- Non-condition effects (constant/ramp/periodic) --------- */

int32_t Calculator::calcNonConditionEffectForce(Effect* effect) {
    int32_t force_vector = 0;
    int32_t magnitude = effect->magnitude;

    if (effect->useEnvelope) {
        magnitude = getEnvelopeMagnitude(effect);
    }

    switch (effect->type) {

    case FFB_EFFECT_CONSTANT: {
        force_vector = static_cast<int32_t>(magnitude);
        break;
    }

    case FFB_EFFECT_RAMP: {
        float elapsed_time = (time_source.micros() / 1000.0f) -
                              static_cast<float>(effect->startTime);
        int32_t duration = effect->duration;
        force_vector = static_cast<int32_t>(effect->startLevel) +
            (static_cast<int32_t>(elapsed_time) *
             (effect->endLevel - effect->startLevel)) / duration;
        break;
    }

    case FFB_EFFECT_SQUARE: {
        uint32_t elapsed_time = time_source.millis() - effect->startTime;
        int32_t force = ((elapsed_time + effect->phase) %
                         (static_cast<uint32_t>(effect->period) + 2))
                        < (static_cast<uint32_t>(effect->period) + 2) / 2
                          ? -magnitude : magnitude;
        force_vector = force + effect->offset;
        break;
    }

    case FFB_EFFECT_TRIANGLE: {
        int32_t force = 0;
        int32_t offset = effect->offset;
        float elapsed_time = time_source.micros() -
                              (static_cast<float>(effect->startTime) * 1000.0f);
        uint32_t phase = effect->phase;
        uint32_t period = effect->period;
        float periodF = period;

        int32_t maxMagnitude = offset + magnitude;
        int32_t minMagnitude = offset - magnitude;
        float phasetime = (phase * period) / 35999.0f;
        uint32_t timeTemp = static_cast<uint32_t>(elapsed_time) +
                              static_cast<uint32_t>(phasetime * 1000.0f);
        float remainder = (timeTemp % (period * 1000)) / 1000.0f;
        float slope = ((maxMagnitude - minMagnitude) * 2) / periodF;
        if (remainder > (periodF / 2)) {
            force = static_cast<int32_t>(slope * (periodF - remainder));
        } else {
            force = static_cast<int32_t>(slope * remainder);
        }
        force += minMagnitude;
        force_vector = force;
        break;
    }

    case FFB_EFFECT_SAWTOOTHUP: {
        float offset = effect->offset;
        float elapsed_time = time_source.micros() -
                              (static_cast<float>(effect->startTime) * 1000.0f);
        uint32_t phase = effect->phase;
        uint32_t period = effect->period;
        float periodF = effect->period;

        float maxMagnitude = offset + magnitude;
        float minMagnitude = offset - magnitude;
        float phasetime = (phase * period) / 35999.0f;
        uint32_t timeTemp = static_cast<uint32_t>(elapsed_time) +
                              static_cast<uint32_t>(phasetime * 1000.0f);
        float remainder = (timeTemp % (period * 1000)) / 1000.0f;
        float slope = (maxMagnitude - minMagnitude) / periodF;
        force_vector = static_cast<int32_t>(minMagnitude +
                                            slope * (period - remainder));
        break;
    }

    case FFB_EFFECT_SAWTOOTHDOWN: {
        float offset = effect->offset;
        float elapsed_time = time_source.micros() -
                              (static_cast<float>(effect->startTime) * 1000.0f);
        float phase = effect->phase;
        uint32_t period = effect->period;
        float periodF = effect->period;

        float maxMagnitude = offset + magnitude;
        float minMagnitude = offset - magnitude;
        float phasetime = (phase * period) / 35999.0f;
        uint32_t timeTemp = static_cast<uint32_t>(elapsed_time) +
                              static_cast<uint32_t>(phasetime * 1000.0f);
        float remainder = (timeTemp % (period * 1000)) / 1000.0f;
        float slope = (maxMagnitude - minMagnitude) / periodF;
        force_vector = static_cast<int32_t>(minMagnitude + slope * remainder);
        break;
    }

    case FFB_EFFECT_SINE: {
        float t = (time_source.micros() / 1000.0f) -
                   static_cast<float>(effect->startTime);
        uint16_t period_for_freq = effect->period;
        if (period_for_freq < 2) period_for_freq = 2;
        float freq = 1.0f / static_cast<float>(period_for_freq);
        float phase = static_cast<float>(effect->phase) / 35999.0f;
        float sine = std::sin(2.0f * static_cast<float>(M_PI) *
                              (t * freq + phase)) * magnitude;
        force_vector = static_cast<int32_t>(effect->offset + sine);
        break;
    }

    default:
        return 0;
    }

    return (force_vector * effect->gain) / 255;
}

/* ----------- Per-axis routing and condition effects ----------------- */

int32_t Calculator::calcComponentForce(Effect* effect, int32_t forceVector, uint8_t axis) {
    int32_t result_torque = 0;
    uint8_t con_idx = effect->useSingleCondition ? 0 : axis;

    AxisState& m = axis_state[axis];
    float angle_ratio = effect->axisMagnitudes[axis];

    switch (effect->type) {
    case FFB_EFFECT_CONSTANT: {
        if (effect->filter_active[axis]) {
            if (effect->filter[axis].getFc() < 0.5f &&
                effect->filter[0].getFc() != 0.0f)
            {
                forceVector = static_cast<int32_t>(effect->filter[axis].process(
                                                       static_cast<float>(forceVector)));
            }
        }
    }
    /* fallthrough: filter is a constant-force preprocessing step */
#if defined(__GNUC__) && __GNUC__ >= 7
        __attribute__((fallthrough));
#endif
    case FFB_EFFECT_RAMP:
    case FFB_EFFECT_SQUARE:
    case FFB_EFFECT_TRIANGLE:
    case FFB_EFFECT_SAWTOOTHUP:
    case FFB_EFFECT_SAWTOOTHDOWN:
    case FFB_EFFECT_SINE:
        result_torque = static_cast<int32_t>(-forceVector * angle_ratio);
        break;

    case FFB_EFFECT_SPRING: {
        float pos = static_cast<float>(m.pos_scaled_16b);
        result_torque -= calcConditionEffectForce(effect, pos,
                                                   gain.spring, con_idx,
                                                   scaler.spring, angle_ratio);
        break;
    }

    case FFB_EFFECT_FRICTION: {
        float speed = m.speed * INTERNAL_SCALER_FRICTION;

        int16_t offset   = effect->conditions[con_idx].cpOffset;
        int16_t deadBand = effect->conditions[con_idx].deadBand;
        int32_t force    = 0;

        float speedRampupCeil = speedRampupPct();

        if (std::abs(static_cast<int32_t>(speed) - offset) > deadBand) {

            speed -= (offset + (deadBand * (speed < offset ? -1 : 1)));

            float rampupFactor = 1.0f;
            if (std::fabs(speed) < speedRampupCeil) {
                float phaseRad = static_cast<float>(M_PI) *
                                  ((std::fabs(speed) / speedRampupCeil) - 0.5f);
                rampupFactor = (1.0f + std::sin(phaseRad)) / 2.0f;
            }

            int8_t  sign  = speed >= 0 ? 1 : -1;
            uint16_t coeff = speed < 0
                ? effect->conditions[con_idx].negativeCoefficient
                : effect->conditions[con_idx].positiveCoefficient;
            force = static_cast<int32_t>(coeff * rampupFactor * sign);

            if (effect->conditions[con_idx].negativeSaturation != 0 ||
                effect->conditions[con_idx].positiveSaturation != 0)
            {
                force = clip_t<int32_t>(
                    force,
                    -static_cast<int32_t>(effect->conditions[con_idx].negativeSaturation),
                     static_cast<int32_t>(effect->conditions[con_idx].positiveSaturation));
            }

            if (effect->filter_active[axis]) {
                result_torque -= static_cast<int32_t>(
                    effect->filter[axis].process(
                        (((gain.friction + 1) * force) >> 8) *
                        angle_ratio * scaler.friction));
            } else {
                result_torque -= static_cast<int32_t>(
                    (((gain.friction + 1) * force) >> 8) *
                    angle_ratio * scaler.friction);
            }
        }
        break;
    }

    case FFB_EFFECT_DAMPER: {
        float speed = m.speed * INTERNAL_SCALER_DAMPER;
        int32_t cf = calcConditionEffectForce(effect, speed, gain.damper,
                                               con_idx, scaler.damper, angle_ratio);
        if (effect->filter_active[axis]) {
            result_torque -= static_cast<int32_t>(
                effect->filter[axis].process(static_cast<float>(cf)));
        } else {
            result_torque -= cf;
        }
        break;
    }

    case FFB_EFFECT_INERTIA: {
        float accel = m.accel * INTERNAL_SCALER_INERTIA;
        int32_t cf = calcConditionEffectForce(effect, accel, gain.inertia,
                                               con_idx, scaler.inertia, angle_ratio);
        if (effect->filter_active[axis]) {
            result_torque -= static_cast<int32_t>(
                effect->filter[axis].process(static_cast<float>(cf)));
        } else {
            result_torque -= cf;
        }
        break;
    }

    default:
        break;
    }

    return (result_torque * global_gain) / 255;
}

float Calculator::speedRampupPct() const {
    return (frictionPctSpeedToRampup / 100.0f) * 32767.0f;
}

int32_t Calculator::calcConditionEffectForce(Effect* effect, float metric,
                                              uint8_t gain_val, uint8_t idx,
                                              float scale, float angle_ratio)
{
    int16_t offset   = effect->conditions[idx].cpOffset;
    int16_t deadBand = effect->conditions[idx].deadBand;
    int32_t force    = 0;
    float gainfactor = static_cast<float>(gain_val + 1) / 256.0f;

    if (std::abs(metric - offset) > deadBand) {
        float coefficient = effect->conditions[idx].negativeCoefficient;
        if (metric > offset) {
            coefficient = effect->conditions[idx].positiveCoefficient;
        }
        coefficient /= 32767.0f;  /* 0x7fff */

        metric = metric - (offset + (deadBand * (metric < offset ? -1 : 1)));

        force = clip_t<int32_t>(
            static_cast<int32_t>(coefficient * gainfactor * scale * metric),
            -static_cast<int32_t>(effect->conditions[idx].negativeSaturation),
             static_cast<int32_t>(effect->conditions[idx].positiveSaturation));
    }

    return static_cast<int32_t>(force * angle_ratio);
}

int32_t Calculator::getEnvelopeMagnitude(Effect* effect) {
    if (effect->duration == FFB_EFFECT_DURATION_INFINITE || effect->duration == 0) {
        return effect->magnitude;
    }
    int32_t scaler_v = std::abs(effect->magnitude);
    uint32_t elapsed_time = time_source.millis() - effect->startTime;
    if (elapsed_time < effect->attackTime && effect->attackTime != 0) {
        scaler_v = (scaler_v - effect->attackLevel) * static_cast<int32_t>(elapsed_time);
        scaler_v /= static_cast<int32_t>(effect->attackTime);
        scaler_v += effect->attackLevel;
    }
    if (elapsed_time > (effect->duration - effect->fadeTime) && effect->fadeTime != 0) {
        scaler_v = (scaler_v - effect->fadeLevel) *
                   (static_cast<int32_t>(effect->duration) - static_cast<int32_t>(elapsed_time));
        scaler_v /= static_cast<int32_t>(effect->fadeTime);
        scaler_v += effect->fadeLevel;
    }
    if (effect->magnitude < 0) scaler_v = -scaler_v;
    return scaler_v;
}

/* ----------- Per-effect biquad filter setup ------------------------- */

void Calculator::setFilters(Effect* effect) {
    EffectFilterPreset& fp = filter_presets[filterProfileId];

    switch (effect->type) {
    case FFB_EFFECT_DAMPER:
        for (int i = 0; i < FFB_MAX_AXIS; ++i) {
            effect->filter[i].setBiquad(BiquadType::lowpass,
                                         fp.damper.freq / calcfrequency,
                                         fp.damper.q * qfloatScaler, 0.0f);
            effect->filter_active[i] = true;
        }
        break;
    case FFB_EFFECT_FRICTION:
        for (int i = 0; i < FFB_MAX_AXIS; ++i) {
            effect->filter[i].setBiquad(BiquadType::lowpass,
                                         fp.friction.freq / calcfrequency,
                                         fp.friction.q * qfloatScaler, 0.0f);
            effect->filter_active[i] = true;
        }
        break;
    case FFB_EFFECT_INERTIA:
        for (int i = 0; i < FFB_MAX_AXIS; ++i) {
            effect->filter[i].setBiquad(BiquadType::lowpass,
                                         fp.inertia.freq / calcfrequency,
                                         fp.inertia.q * qfloatScaler, 0.0f);
            effect->filter_active[i] = true;
        }
        break;
    case FFB_EFFECT_CONSTANT:
        for (int i = 0; i < FFB_MAX_AXIS; ++i) {
            /* Constant-force filter always uses profile 0, matching original */
            effect->filter[i].setBiquad(BiquadType::lowpass,
                                         filter_presets[0].constant.freq / calcfrequency,
                                         filter_presets[0].constant.q * qfloatScaler, 0.0f);
            effect->filter_active[i] = true;
        }
        break;
    default:
        /* Other effect types have no per-effect filter. */
        break;
    }
}

void Calculator::updateFiltersForType(uint8_t effect_type) {
    for (uint8_t i = 0; i < FFB_MAX_EFFECTS; ++i) {
        if (effects[i].type == effect_type) {
            setFilters(&effects[i]);
        }
    }
}

} /* namespace ffb */
