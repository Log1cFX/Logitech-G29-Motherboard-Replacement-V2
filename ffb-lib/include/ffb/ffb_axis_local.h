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
 * ffb_axis_local.h  (optional)
 *
 * Axis-local "feel" effects that are NOT requested by the USB host:
 *
 *   - idle spring (auto-centring when FFB is disabled)
 *   - software end-stop (extra torque when the wheel hits the rotation limit)
 *   - always-on damper / friction / inertia
 *
 * These are taken from OpenFFBoard's Axis::calculateAxisEffects() and
 * Axis::updateEndstop(). Most users of the standalone library will not
 * need this header - skip it if you only want to relay host-requested
 * effects.
 */

#ifndef FFB_AXIS_LOCAL_H_
#define FFB_AXIS_LOCAL_H_

#include <cstdint>

#include "ffb/ffb_biquad.h"
#include "ffb/ffb_calculator.h"

namespace ffb {

struct AxisLocalConfig {
    /* Idle spring (active when FFB is off). 0..255. */
    uint8_t idle_spring_strength = 0;

    /* Endstop intensity. 0..255. Higher = stiffer. */
    uint8_t endstop_strength     = 127;

    /* Always-on axis-level effects. 0..255. */
    uint8_t damper_intensity     = 30;
    uint8_t friction_intensity   = 0;
    uint8_t inertia_intensity    = 0;

    /* Wheel range (full rotation in degrees). */
    float   degrees_of_rotation  = 900.0f;

    /* Filter coefficients - defaults match Axis::filter*Cst. */
    biquad_constant_t damper_filter   = { 60, 55 };
    biquad_constant_t friction_filter = { 50, 20 };
    biquad_constant_t inertia_filter  = { 20, 20 };

    float samplerate_hz = FFB_DEFAULT_SAMPLERATE_HZ;
};

class AxisLocalEffects {
public:
    explicit AxisLocalEffects(const AxisLocalConfig& cfg = AxisLocalConfig{});

    /* Compute the per-axis "feel" torque to add on top of HID-requested
     * effects. Returns a value in -0x7fff..0x7fff. Add this to
     * Library::getAxisTorque(axis) before applying to the motor. */
    int32_t compute(const AxisState& metrics, float pos_degrees, bool ffb_on);

    /* Update samplerate (rebuilds filter coefficients). */
    void setSamplerate(float hz);

    /* Retune the idle-spring strength at runtime. Needed because the
     * derived scale/clip values are cached at construction; writing
     * config().idle_spring_strength directly would not take effect. */
    void setIdleSpringStrength(uint8_t strength);

    AxisLocalConfig& config() { return cfg; }

private:
    int32_t updateIdleSpring(int32_t pos_scaled_16b) const;
    int32_t updateEndstop(int32_t pos_scaled_16b, float pos_degrees) const;

    AxisLocalConfig cfg;
    Biquad damper_filter;
    Biquad friction_filter;
    Biquad inertia_filter;
    float  idle_spring_scale = 0;
    int32_t idle_spring_clip = 0;
};

} /* namespace ffb */

#endif /* FFB_AXIS_LOCAL_H_ */
