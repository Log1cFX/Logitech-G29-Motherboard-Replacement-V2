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
 * ffb_axis_local_c.cpp
 *
 * C wrapper for ffb::AxisLocalEffects. Instances live in a static pool of
 * FFB_MAX_AXIS slots (no heap), constructed in place by
 * ffb_axis_local_create().
 *
 * The opaque ffb_axis_local_t* the caller holds is the address of an
 * AxisLocalEffects inside the pool; as_a() casts it back. The only real work
 * here is translating between the flat plain-C config struct (which splits each
 * {freq,q} filter pair into two members) and the C++ AxisLocalConfig.
 */

#include "ffb/ffb_axis_local_c.h"

#include "ffb/ffb_axis_local.h"

#include <new>   /* placement new */

namespace {

/* One slot per axis, no heap. */
alignas(ffb::AxisLocalEffects)
unsigned char g_storage[FFB_MAX_AXIS][sizeof(ffb::AxisLocalEffects)];
unsigned g_count = 0;   /* slots handed out so far */

/* Opaque handle -> real object. */
inline ffb::AxisLocalEffects* as_a(ffb_axis_local_t* h) {
    return reinterpret_cast<ffb::AxisLocalEffects*>(h);
}

} /* anonymous namespace */

extern "C" {

/* Fill *out with the library defaults so the caller can tweak a few fields and
 * pass it to ffb_axis_local_create() without zero-initialising everything. The
 * values come straight from a default-constructed C++ AxisLocalConfig. */
void ffb_axis_local_config_default(ffb_axis_local_config_t* out) {
    if (!out) return;
    ffb::AxisLocalConfig d;   /* C++ defaults */
    out->idle_spring_strength = d.idle_spring_strength;
    out->endstop_strength     = d.endstop_strength;
    out->damper_intensity     = d.damper_intensity;
    out->friction_intensity   = d.friction_intensity;
    out->inertia_intensity    = d.inertia_intensity;
    out->degrees_of_rotation  = d.degrees_of_rotation;
    /* Flatten each {freq,q} biquad pair into the two flat C members. */
    out->damper_filter_freq   = d.damper_filter.freq;   out->damper_filter_q   = d.damper_filter.q;
    out->friction_filter_freq = d.friction_filter.freq; out->friction_filter_q = d.friction_filter.q;
    out->inertia_filter_freq  = d.inertia_filter.freq;  out->inertia_filter_q  = d.inertia_filter.q;
    out->samplerate_hz        = d.samplerate_hz;
}

/* Allocate one axis-local unit from the static pool. Pass NULL for cfg to use
 * the defaults. Returns NULL once all FFB_MAX_AXIS slots are in use. */
ffb_axis_local_t* ffb_axis_local_create(const ffb_axis_local_config_t* cfg) {
    if (g_count >= FFB_MAX_AXIS) {
        return nullptr;   /* pool full - one unit per axis */
    }
    ffb::AxisLocalConfig c;   /* starts at C++ defaults */
    if (cfg) {
        /* Copy the flat C config into the C++ one, re-pairing the filters. */
        c.idle_spring_strength = cfg->idle_spring_strength;
        c.endstop_strength     = cfg->endstop_strength;
        c.damper_intensity     = cfg->damper_intensity;
        c.friction_intensity   = cfg->friction_intensity;
        c.inertia_intensity    = cfg->inertia_intensity;
        c.degrees_of_rotation  = cfg->degrees_of_rotation;
        c.damper_filter.freq   = cfg->damper_filter_freq;   c.damper_filter.q   = cfg->damper_filter_q;
        c.friction_filter.freq = cfg->friction_filter_freq; c.friction_filter.q = cfg->friction_filter_q;
        c.inertia_filter.freq  = cfg->inertia_filter_freq;  c.inertia_filter.q  = cfg->inertia_filter_q;
        c.samplerate_hz        = cfg->samplerate_hz;
    }
    void* slot = g_storage[g_count++];
    ffb::AxisLocalEffects* a = new (slot) ffb::AxisLocalEffects(c);
    return reinterpret_cast<ffb_axis_local_t*>(a);
}

/* Compute the axis-local "feel" torque to ADD on top of the host torque.
 * Unpacks the C axis state, then forwards to AxisLocalEffects::compute(). */
int32_t ffb_axis_local_compute(ffb_axis_local_t* a, const ffb_axis_state_t* metrics,
                               float pos_degrees, bool ffb_on) {
    if (!a || !metrics) return 0;
    ffb::AxisState s;
    s.pos_scaled_16b = metrics->pos_scaled_16b;
    s.speed          = metrics->speed;
    s.accel          = metrics->accel;
    return as_a(a)->compute(s, pos_degrees, ffb_on);
}

/* Rebuild the damper/friction/inertia filter coefficients for a new rate. */
void ffb_axis_local_set_samplerate(ffb_axis_local_t* a, float hz) {
    if (!a) return;
    as_a(a)->setSamplerate(hz);
}

/* Retune the idle spring. Needs its own setter (not just a config write)
 * because the derived scale/clip values are cached and must be recomputed. */
void ffb_axis_local_set_idle_spring(ffb_axis_local_t* a, uint8_t strength) {
    if (!a) return;
    as_a(a)->setIdleSpringStrength(strength);
}

/* Retune the four live intensities at once. These are read on every compute()
 * so they take effect on the next tick; pass the current value for any field
 * you want to leave unchanged (there are no per-field setters in C). */
void ffb_axis_local_set_intensities(ffb_axis_local_t* a,
                                    uint8_t endstop_strength,
                                    uint8_t damper_intensity,
                                    uint8_t friction_intensity,
                                    uint8_t inertia_intensity) {
    if (!a) return;
    ffb::AxisLocalConfig& c = as_a(a)->config();
    c.endstop_strength   = endstop_strength;
    c.damper_intensity   = damper_intensity;
    c.friction_intensity = friction_intensity;
    c.inertia_intensity  = inertia_intensity;
}

} /* extern "C" */
