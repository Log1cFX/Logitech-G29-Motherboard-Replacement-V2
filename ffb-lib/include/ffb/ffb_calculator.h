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
 * ffb_calculator.h
 *
 * Computes per-axis FFB torque every tick. Ported from
 * OpenFFBoard EffectsCalculator. The math (calcNonConditionEffectForce,
 * calcComponentForce, calcConditionEffectForce, getEnvelopeMagnitude,
 * setFilters) is bit-identical to the original; the differences are:
 *
 *   - Inheritance from Thread / PersistentStorage / CommandHandler removed.
 *   - The `axes` vector parameter is replaced by an internal AxisState
 *     buffer (filled via setAxisState) and a torque output buffer (read
 *     via getAxisTorque).
 *   - HAL_GetTick()/micros() replaced by the user-supplied TimeSource.
 *   - Persistent flash storage stripped (filter coefficients are runtime
 *     defaults that match the original firmware's defaults).
 *   - Statistics, monitor thread, command interface removed.
 */

#ifndef FFB_CALCULATOR_H_
#define FFB_CALCULATOR_H_

#include <array>
#include <cstdint>

#include "ffb/ffb_biquad.h"
#include "ffb/ffb_config.h"
#include "ffb/ffb_defs.h"
#include "ffb/ffb_effect.h"

namespace ffb {

/* Per-axis kinematic state, filled by the user before calling calculate(). */
struct AxisState {
    int32_t pos_scaled_16b;  /* -0x7fff .. 0x7fff */
    float   speed;            /* deg/s             */
    float   accel;            /* deg/s^2           */
    AxisState() : pos_scaled_16b(0), speed(0), accel(0) {}
    AxisState(int32_t p, float s, float a)
        : pos_scaled_16b(p), speed(s), accel(a) {}
};

/* User-supplied time source (millisecond + microsecond counters). */
struct TimeSource {
    uint32_t (*millis)();
    uint32_t (*micros)();
    TimeSource() : millis(nullptr), micros(nullptr) {}
    TimeSource(uint32_t (*m)(), uint32_t (*u)()) : millis(m), micros(u) {}
};

/* Default effect gains. Match OpenFFBoard's effect_gain_t defaults. */
struct EffectGain {
    uint8_t friction = 254;
    uint8_t spring   = 64;
    uint8_t damper   = 64;
    uint8_t inertia  = 127;
};

/* Per-effect-type scaling factors. Match OpenFFBoard's effect_scaler_t. */
struct EffectScaler {
    float friction = 1.0f;
    float spring   = 16.0f;
    float damper   = 4.0f;
    float inertia  = 2.0f;
};

/* Per-effect-type biquad coefficients. Match OpenFFBoard's effect_biquad_t. */
struct EffectFilterPreset {
    biquad_constant_t constant = { 500, 70 };
    biquad_constant_t friction = { 50,  20 };
    biquad_constant_t damper   = { 30,  40 };
    biquad_constant_t inertia  = { 15,  20 };
};

class Calculator {
public:
    Calculator(uint8_t axis_count, TimeSource ts);

    static constexpr uint32_t INTERNAL_SCALER_DAMPER   = 40;
    static constexpr uint32_t INTERNAL_SCALER_FRICTION = 45;
    static constexpr uint32_t INTERNAL_SCALER_INERTIA  = 4;

    /* Run one tick: read axis_state[], write axis_torque[]. */
    void calculate();

    /* Public effect array. The HID parser shares this. */
    std::array<Effect, FFB_MAX_EFFECTS> effects;

    /* Effect slot management. */
    int32_t findFreeEffect(uint8_t type);
    void    freeEffect(uint16_t idx);

    /* Filter initialisation for a single effect (call when an effect is
     * created or its type changes). */
    void setFilters(Effect* effect);

    /* Axis state input / torque output. */
    void    setAxisState(uint8_t axis, const AxisState& s);
    int32_t getAxisTorque(uint8_t axis) const;
    uint8_t getAxisCount() const { return axis_count; }

    /* Control. */
    bool isActive() const { return effects_active; }
    void setActive(bool a) { effects_active = a; }

    /* Settings (match the configurator's runtime-settable knobs). */
    void    setGlobalGain(uint8_t g) { global_gain = g; }
    uint8_t getGlobalGain() const    { return global_gain; }

    void  setSamplerate(float hz);
    float getSamplerate() const { return calcfrequency; }

    /* Time-source pass-through for the parser (which needs to stamp
     * effect start times in millis). */
    uint32_t millisNow() const { return time_source.millis ? time_source.millis() : 0; }
    uint32_t microsNow() const { return time_source.micros ? time_source.micros() : 0; }

    void setFrictionRampupPct(uint8_t pct);
    uint8_t getFrictionRampupPct() const { return frictionPctSpeedToRampup; }

    /* Filter profile selector (0 default presets, 1 custom). The custom
     * profile starts as the same value as the default. */
    void setFilterProfileId(uint8_t id);
    uint8_t getFilterProfileId() const { return filterProfileId; }

    /* Direct access to the gain/scaler/filter tables (mostly for advanced
     * users; the defaults already match the original firmware). */
    EffectGain&         gains()       { return gain; }
    EffectScaler&       scalers()     { return scaler; }
    EffectFilterPreset& filterPreset(uint8_t profile);

    /* Rebuilds filter coefficients on every effect of the given type.
     * Call after changing samplerate or filter preset. */
    void updateFiltersForType(uint8_t effect_type);

private:
    /* Math — copied verbatim from EffectsCalculator.cpp. */
    int32_t calcNonConditionEffectForce(Effect* effect);
    int32_t calcComponentForce(Effect* effect, int32_t forceVector, uint8_t axis);
    int32_t calcConditionEffectForce(Effect* effect, float metric,
                                     uint8_t gain_val, uint8_t idx,
                                     float scale, float angle_ratio);
    int32_t getEnvelopeMagnitude(Effect* effect);
    float   speedRampupPct() const;

    /* Configuration. */
    uint8_t      axis_count;
    TimeSource   time_source;
    bool         effects_active = false;
    uint8_t      global_gain    = 0xff;
    float        calcfrequency  = FFB_DEFAULT_SAMPLERATE_HZ;
    EffectGain   gain;
    EffectScaler scaler;
    EffectFilterPreset filter_presets[2];      /* 0 = default, 1 = custom */
    uint8_t      filterProfileId = 0;
    uint8_t      frictionPctSpeedToRampup = 25;
    const float  qfloatScaler = 0.01f;

    /* Per-axis state, filled by setAxisState() / read by calculate(). */
    AxisState axis_state[FFB_MAX_AXIS];
    int32_t   axis_torque[FFB_MAX_AXIS] = {0};
};

} /* namespace ffb */

#endif /* FFB_CALCULATOR_H_ */
