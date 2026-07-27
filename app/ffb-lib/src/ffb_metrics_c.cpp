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
 * ffb_metrics_c.cpp
 *
 * C wrapper for ffb::MetricsBuilder. Instances live in a static pool of
 * FFB_MAX_AXIS slots (no heap), constructed in place by ffb_metrics_create().
 *
 * Each builder turns a stream of raw wheel angles (degrees) into the scaled
 * position + filtered speed/accel the engine wants. The opaque ffb_metrics_t*
 * the caller holds is the address of a MetricsBuilder inside the pool; as_m()
 * casts it back, and every wrapper null-checks then forwards.
 */

#include "ffb/ffb_metrics_c.h"

#include "ffb/ffb.h"
#include "ffb/ffb_metrics.h"

#include <new>   /* placement new */

namespace {

/* One slot per axis, no heap. Aligned and sized for a MetricsBuilder. */
alignas(ffb::MetricsBuilder)
unsigned char g_storage[FFB_MAX_AXIS][sizeof(ffb::MetricsBuilder)];
unsigned g_count = 0;   /* how many slots are handed out so far */

/* Opaque handle -> real object. */
inline ffb::MetricsBuilder* as_m(ffb_metrics_t* h) {
    return reinterpret_cast<ffb::MetricsBuilder*>(h);
}

/* Shared constructor body for both create() entry points: grab the next free
 * pool slot (or fail if the pool is exhausted) and placement-new into it. */
ffb_metrics_t* make(float degrees, float hz, ffb::MetricsFilterPreset preset) {
    if (g_count >= FFB_MAX_AXIS) {
        return nullptr;   /* pool full - one builder per axis */
    }
    void* slot = g_storage[g_count++];
    ffb::MetricsBuilder* m = new (slot) ffb::MetricsBuilder(degrees, hz, preset);
    return reinterpret_cast<ffb_metrics_t*>(m);
}

} /* anonymous namespace */

extern "C" {

/* Create a builder with the default speed/accel filters ({70,55} / {55,30}). */
ffb_metrics_t* ffb_metrics_create(float degrees_of_rotation, float samplerate_hz) {
    return make(degrees_of_rotation, samplerate_hz, ffb::MetricsFilterPreset{});
}

/* Same, but with caller-chosen low-pass coefficients (freq in Hz, q is Q*100). */
ffb_metrics_t* ffb_metrics_create_ex(float degrees_of_rotation, float samplerate_hz,
                                     uint16_t speed_freq, uint8_t speed_q,
                                     uint16_t accel_freq, uint8_t accel_q) {
    ffb::MetricsFilterPreset preset;
    preset.speed.freq = speed_freq; preset.speed.q = speed_q;
    preset.accel.freq = accel_freq; preset.accel.q = accel_q;
    return make(degrees_of_rotation, samplerate_hz, preset);
}

/* Push a new raw angle and return the resulting axis state. The C++ AxisState
 * is copied field-by-field into the plain-C ffb_axis_state_t. */
ffb_axis_state_t ffb_metrics_update(ffb_metrics_t* m, float new_pos_degrees) {
    ffb_axis_state_t out = {0, 0.0f, 0.0f};
    if (!m) return out;
    ffb::AxisState s = as_m(m)->update(new_pos_degrees);
    out.pos_scaled_16b = s.pos_scaled_16b;
    out.speed          = s.speed;
    out.accel          = s.accel;
    return out;
}

/* Convenience: update the builder and feed the result straight into the
 * engine axis in one call (the common case when you don't need the state). */
void ffb_metrics_update_and_set(ffb_metrics_t* m, ffb_lib_t* lib,
                                uint8_t axis, float new_pos_degrees) {
    if (!m || !lib) return;
    ffb_axis_state_t s = ffb_metrics_update(m, new_pos_degrees);
    ffb_set_axis_state_s(lib, axis, &s);
}

/* Re-derive the filter coefficients for a new control-loop rate. */
void ffb_metrics_set_samplerate(ffb_metrics_t* m, float hz) {
    if (!m) return;
    as_m(m)->setSamplerate(hz);
}

/* Re-seed history at a known position (clears speed/accel so the next update
 * doesn't see a huge bogus jump). */
void ffb_metrics_reset(ffb_metrics_t* m, float pos_degrees) {
    if (!m) return;
    as_m(m)->reset(pos_degrees);
}

} /* extern "C" */
