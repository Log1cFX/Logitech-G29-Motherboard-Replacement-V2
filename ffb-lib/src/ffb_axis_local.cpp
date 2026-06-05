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
 * ffb_axis_local.cpp
 *
 * Logic ported from OpenFFBoard Axis::calculateAxisEffects() and
 * Axis::updateEndstop(). Bit-for-bit equivalent at the math layer.
 */

#include "ffb/ffb_axis_local.h"

#include <cmath>

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

/* Match OpenFFBoard's INTERNAL_*_SCALER (effects calculator side). */
constexpr float AXIS_DAMPER_RATIO  = 40.0f * 0.7f / 255.0f;   /* DAMPER * 0.7 / 255 */
constexpr float AXIS_INERTIA_RATIO = 4.0f  * 0.7f / 255.0f;   /* INERTIA * 0.7 / 255 */
constexpr float INTERNAL_AXIS_FRICTION_SCALER = 0.7f;
constexpr int32_t INT_FX_CLIP = 20000;
constexpr float ENDSTOP_GAIN = 25.0f;

constexpr int32_t INTERNAL_SCALER_FRICTION_LOCAL = 45;

template <typename T>
inline int8_t cliptest(T v, T lo, T hi) {
    if (v < lo) return -1;
    if (v > hi) return 1;
    return 0;
}

} /* anonymous namespace */

namespace ffb {

AxisLocalEffects::AxisLocalEffects(const AxisLocalConfig& c) : cfg(c) {
    setSamplerate(c.samplerate_hz);
    setIdleSpringStrength(cfg.idle_spring_strength);
}

void AxisLocalEffects::setIdleSpringStrength(uint8_t strength) {
    /* Idle spring scale matches Axis::setIdleSpringStrength */
    cfg.idle_spring_strength = strength;
    idle_spring_clip  = clip_t<int32_t>(static_cast<int32_t>(strength) * 35, 0, 10000);
    idle_spring_scale = 0.5f + (static_cast<float>(strength) * 0.01f);
}

void AxisLocalEffects::setSamplerate(float hz) {
    if (hz <= 0.0f) hz = FFB_DEFAULT_SAMPLERATE_HZ;
    cfg.samplerate_hz = hz;
    damper_filter.setBiquad(BiquadType::lowpass,
                             cfg.damper_filter.freq / hz,
                             cfg.damper_filter.q / 100.0f, 0.0f);
    friction_filter.setBiquad(BiquadType::lowpass,
                               cfg.friction_filter.freq / hz,
                               cfg.friction_filter.q / 100.0f, 0.0f);
    inertia_filter.setBiquad(BiquadType::lowpass,
                              cfg.inertia_filter.freq / hz,
                              cfg.inertia_filter.q / 100.0f, 0.0f);
}

int32_t AxisLocalEffects::updateIdleSpring(int32_t pos_scaled_16b) const {
    int32_t f = static_cast<int32_t>(-pos_scaled_16b * idle_spring_scale);
    return clip_t<int32_t>(f, -idle_spring_clip, idle_spring_clip);
}

int32_t AxisLocalEffects::updateEndstop(int32_t pos_scaled_16b, float pos_degrees) const {
    int8_t clipdir = cliptest<int32_t>(pos_scaled_16b, -0x7fff, 0x7fff);
    if (clipdir == 0) return 0;
    float addtorque = clipdir * pos_degrees - (cfg.degrees_of_rotation / 2.0f);
    addtorque *= static_cast<float>(cfg.endstop_strength) * ENDSTOP_GAIN;
    addtorque *= -clipdir;
    return clip_t<int32_t>(static_cast<int32_t>(addtorque), -0x7fff, 0x7fff);
}

int32_t AxisLocalEffects::compute(const AxisState& m, float pos_degrees, bool ffb_on) {
    int32_t axisEffectTorque = 0;

    if (!ffb_on) {
        axisEffectTorque += updateIdleSpring(m.pos_scaled_16b);
    }

    if (cfg.damper_intensity != 0) {
        float speedFiltered = m.speed * static_cast<float>(cfg.damper_intensity) * AXIS_DAMPER_RATIO;
        speedFiltered = clip_t<float>(speedFiltered,
                                       static_cast<float>(-INT_FX_CLIP),
                                       static_cast<float>(INT_FX_CLIP));
        axisEffectTorque -= static_cast<int32_t>(damper_filter.process(speedFiltered));
    }

    if (cfg.inertia_intensity != 0) {
        float accelFiltered = m.accel * static_cast<float>(cfg.inertia_intensity) * AXIS_INERTIA_RATIO;
        accelFiltered = clip_t<float>(accelFiltered,
                                       static_cast<float>(-INT_FX_CLIP),
                                       static_cast<float>(INT_FX_CLIP));
        axisEffectTorque -= static_cast<int32_t>(inertia_filter.process(accelFiltered));
    }

    if (cfg.friction_intensity != 0) {
        float speed = m.speed * static_cast<float>(INTERNAL_SCALER_FRICTION_LOCAL);
        float speedRampupCeil = 4096.0f;
        float rampupFactor = 1.0f;
        if (std::fabs(speed) < speedRampupCeil) {
            float phaseRad = static_cast<float>(M_PI) *
                              ((std::fabs(speed) / speedRampupCeil) - 0.5f);
            rampupFactor = (1.0f + std::sin(phaseRad)) / 2.0f;
        }
        int8_t sign = speed >= 0 ? 1 : -1;
        float force = static_cast<float>(cfg.friction_intensity) * rampupFactor *
                       sign * INTERNAL_AXIS_FRICTION_SCALER * 32.0f;
        force = clip_t<float>(force,
                               static_cast<float>(-INT_FX_CLIP),
                               static_cast<float>(INT_FX_CLIP));
        axisEffectTorque -= static_cast<int32_t>(friction_filter.process(force));
    }

    /* Endstop. */
    axisEffectTorque += updateEndstop(m.pos_scaled_16b, pos_degrees);

    return clip_t<int32_t>(axisEffectTorque, -0x7fff, 0x7fff);
}

} /* namespace ffb */
