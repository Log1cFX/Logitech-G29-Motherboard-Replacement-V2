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
 * ffb_metrics_c.h  (optional)
 *
 * Plain-C interface to ffb::MetricsBuilder - derives filtered speed and
 * acceleration (and a scaled position) from a raw wheel position stream.
 *
 * This is an OPT-IN helper: it lives in its own translation unit
 * (src/ffb_metrics_c.cpp). If you never include this header and never link
 * that file, it contributes zero bytes. Compile src/ffb_metrics_c.cpp and
 * src/ffb_metrics.cpp when you use it.
 *
 * Storage is a small static pool (one slot per FFB_MAX_AXIS, no heap), so
 * ffb_metrics_create() may be called at most FFB_MAX_AXIS times.
 */

#ifndef FFB_METRICS_C_H_
#define FFB_METRICS_C_H_

#include <stdint.h>

#include "ffb/ffb_c.h"   /* ffb_axis_state_t, ffb_lib_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ffb_metrics ffb_metrics_t;

/* Create a metrics builder. degrees_of_rotation is the wheel's full travel
 * (e.g. 900); samplerate_hz is your control-loop rate. Uses the default
 * filter presets (speed {70,55}, accel {55,30}). Returns NULL if the pool
 * (FFB_MAX_AXIS slots) is exhausted. */
ffb_metrics_t* ffb_metrics_create(float degrees_of_rotation, float samplerate_hz);

/* Same, but with explicit speed/accel low-pass filter coefficients
 * (freq in Hz, q is Q*100, e.g. q=55 -> Q=0.55). */
ffb_metrics_t* ffb_metrics_create_ex(float degrees_of_rotation, float samplerate_hz,
                                     uint16_t speed_freq, uint8_t speed_q,
                                     uint16_t accel_freq, uint8_t accel_q);

/* Push a new raw position (degrees) and get back the resulting axis state. */
ffb_axis_state_t ffb_metrics_update(ffb_metrics_t* m, float new_pos_degrees);

/* Convenience: update and feed the result straight into the engine axis. */
void ffb_metrics_update_and_set(ffb_metrics_t* m, ffb_lib_t* lib,
                                uint8_t axis, float new_pos_degrees);

/* Rebuild filter coefficients for a new control-loop rate. */
void ffb_metrics_set_samplerate(ffb_metrics_t* m, float hz);

/* Re-initialise at a position, clearing speed/accel history. */
void ffb_metrics_reset(ffb_metrics_t* m, float pos_degrees);

#ifdef __cplusplus
}
#endif

#endif /* FFB_METRICS_C_H_ */
