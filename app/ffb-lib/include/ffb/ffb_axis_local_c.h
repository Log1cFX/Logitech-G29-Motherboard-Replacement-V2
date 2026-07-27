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
 * ffb_axis_local_c.h  (optional)
 *
 * Plain-C interface to ffb::AxisLocalEffects - the axis-local "feel"
 * effects that are NOT requested by the host: idle self-centring spring,
 * software end-stop, and always-on damper/friction/inertia.
 *
 * OPT-IN helper in its own translation unit (src/ffb_axis_local_c.cpp).
 * Include this header and compile src/ffb_axis_local_c.cpp +
 * src/ffb_axis_local.cpp only if you want it; otherwise it costs nothing.
 *
 * Storage is a static pool (one slot per FFB_MAX_AXIS, no heap), so
 * ffb_axis_local_create() may be called at most FFB_MAX_AXIS times.
 */

#ifndef FFB_AXIS_LOCAL_C_H_
#define FFB_AXIS_LOCAL_C_H_

#include <stdbool.h>
#include <stdint.h>

#include "ffb/ffb_c.h"   /* ffb_axis_state_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ffb_axis_local ffb_axis_local_t;

/* Mirror of ffb::AxisLocalConfig (filter pairs are freq in Hz, q is Q*100). */
typedef struct {
    uint8_t  idle_spring_strength;   /* 0..255, 0 = off                  */
    uint8_t  endstop_strength;       /* 0..255, higher = stiffer wall    */
    uint8_t  damper_intensity;       /* 0..255 always-on damping         */
    uint8_t  friction_intensity;     /* 0..255 always-on friction        */
    uint8_t  inertia_intensity;      /* 0..255 always-on inertia         */
    float    degrees_of_rotation;    /* full wheel travel, e.g. 900      */
    uint16_t damper_filter_freq;   uint8_t damper_filter_q;
    uint16_t friction_filter_freq; uint8_t friction_filter_q;
    uint16_t inertia_filter_freq;  uint8_t inertia_filter_q;
    float    samplerate_hz;          /* control-loop rate                */
} ffb_axis_local_config_t;

/* Fill *out with the same defaults as ffb::AxisLocalConfig{}. */
void ffb_axis_local_config_default(ffb_axis_local_config_t* out);

/* Create an axis-local effects unit. Pass NULL for cfg to use defaults.
 * Returns NULL if the pool (FFB_MAX_AXIS slots) is exhausted. */
ffb_axis_local_t* ffb_axis_local_create(const ffb_axis_local_config_t* cfg);

/* Compute the per-axis "feel" torque (-0x7fff..0x7fff) to ADD on top of
 * the host-requested torque from ffb_get_axis_torque(). metrics is the same
 * axis state you feed the engine; pos_degrees is the raw wheel angle;
 * ffb_on is whether host FFB is active (idle spring engages when it is not). */
int32_t ffb_axis_local_compute(ffb_axis_local_t* a, const ffb_axis_state_t* metrics,
                               float pos_degrees, bool ffb_on);

/* Rebuild filter coefficients for a new control-loop rate. */
void ffb_axis_local_set_samplerate(ffb_axis_local_t* a, float hz);

/* Retune the idle-spring strength at runtime (handles the cached scale). */
void ffb_axis_local_set_idle_spring(ffb_axis_local_t* a, uint8_t strength);

/* Retune the live intensities (damper/friction/inertia/endstop). These are
 * read on every compute() so they take effect immediately. Idle spring has
 * its own setter above. */
void ffb_axis_local_set_intensities(ffb_axis_local_t* a,
                                    uint8_t endstop_strength,
                                    uint8_t damper_intensity,
                                    uint8_t friction_intensity,
                                    uint8_t inertia_intensity);

#ifdef __cplusplus
}
#endif

#endif /* FFB_AXIS_LOCAL_C_H_ */
