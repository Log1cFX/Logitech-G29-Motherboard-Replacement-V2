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
 * ffb_c.h
 *
 * Plain-C interface to the FFB library, for projects whose main code
 * is C or for FFI bindings. The implementation holds a single static
 * ffb::Library instance under the hood, so ffb_create() may be called
 * exactly once per program.
 */

#ifndef FFB_C_H_
#define FFB_C_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ffb_lib ffb_lib_t;

typedef uint32_t (*ffb_time_fn_t)(void);
typedef bool     (*ffb_send_report_fn_t)(const uint8_t* report, uint16_t len);

/* Mirror of ffb::AxisState. Position is the wheel/axis location scaled so
 * that the two travel limits map to -0x7fff..+0x7fff; speed/accel are in
 * deg/s and deg/s^2. Returned by the metrics helper and accepted by the
 * axis-local helper. */
typedef struct {
    int32_t pos_scaled_16b;
    float   speed;
    float   accel;
} ffb_axis_state_t;

/* Per-effect-type master gains (0..255). Mirrors ffb::EffectGain. */
typedef struct {
    uint8_t spring;
    uint8_t damper;
    uint8_t inertia;
    uint8_t friction;
} ffb_effect_gain_t;

/* Per-effect-type output scalers. Mirrors ffb::EffectScaler. */
typedef struct {
    float spring;
    float damper;
    float inertia;
    float friction;
} ffb_effect_scaler_t;

/* Per-effect-type biquad cutoff presets (freq in Hz, q is Q*100).
 * Mirrors ffb::EffectFilterPreset. */
typedef struct {
    uint16_t constant_freq; uint8_t constant_q;
    uint16_t friction_freq; uint8_t friction_q;
    uint16_t damper_freq;   uint8_t damper_q;
    uint16_t inertia_freq;  uint8_t inertia_q;
} ffb_effect_filter_preset_t;

/* Construct the single FFB instance. Returns a handle that survives
 * the program's lifetime. Pass NULL for the send_report callback if
 * you do not need status reports pushed back to the host. */
ffb_lib_t* ffb_create(uint8_t axis_count,
                      ffb_time_fn_t millis_fn,
                      ffb_time_fn_t micros_fn);

void ffb_set_send_report_callback(ffb_lib_t* lib, ffb_send_report_fn_t cb);

/* USB receive (call from your USB stack). */
void     ffb_hid_out(ffb_lib_t* lib, uint8_t report_id,
                     const uint8_t* buf, uint16_t len);
uint16_t ffb_hid_get(ffb_lib_t* lib, uint8_t report_id,
                     uint8_t* reply, uint16_t reqlen);

/* Per-tick loop. */
void    ffb_set_axis_state(ffb_lib_t* lib, uint8_t axis,
                           int32_t pos_scaled_16b, float speed, float accel);
void    ffb_calculate(ffb_lib_t* lib);
int32_t ffb_get_axis_torque(ffb_lib_t* lib, uint8_t axis);

/* Control. */
void ffb_set_active(ffb_lib_t* lib, bool on);
bool ffb_is_active(ffb_lib_t* lib);
void ffb_reset_all_effects(ffb_lib_t* lib);
void ffb_set_global_gain(ffb_lib_t* lib, uint8_t gain);
void ffb_set_samplerate(ffb_lib_t* lib, float hz);
void ffb_set_direction_enable_mask(ffb_lib_t* lib, uint8_t mask);

/* Convenience: push an ffb_axis_state_t (e.g. produced by the metrics
 * helper) straight into the engine. */
void ffb_set_axis_state_s(ffb_lib_t* lib, uint8_t axis,
                          const ffb_axis_state_t* state);

/* Getters that previously had no C equivalent. */
uint8_t ffb_get_global_gain(ffb_lib_t* lib);
float   ffb_get_samplerate(ffb_lib_t* lib);
uint8_t ffb_get_axis_count(ffb_lib_t* lib);

/* Friction ramp-up threshold (percent of full speed below which friction
 * is eased in with a half-sine). Matches the configurator knob. */
void    ffb_set_friction_rampup_pct(ffb_lib_t* lib, uint8_t pct);
uint8_t ffb_get_friction_rampup_pct(ffb_lib_t* lib);

/* Biquad filter profile selector: 0 = built-in defaults, 1 = custom
 * (edit profile 1 with ffb_set_filter_preset, then select it here). */
void    ffb_set_filter_profile_id(ffb_lib_t* lib, uint8_t id);
uint8_t ffb_get_filter_profile_id(ffb_lib_t* lib);

/* Advanced runtime tuning of the condition-effect gain/scaler tables.
 * Defaults already match upstream OpenFFBoard. */
void ffb_set_effect_gains(ffb_lib_t* lib, const ffb_effect_gain_t* gains);
void ffb_get_effect_gains(ffb_lib_t* lib, ffb_effect_gain_t* out);
void ffb_set_effect_scalers(ffb_lib_t* lib, const ffb_effect_scaler_t* scalers);
void ffb_get_effect_scalers(ffb_lib_t* lib, ffb_effect_scaler_t* out);

/* Edit/read a filter preset (profile 0 = default, 1 = custom). After
 * editing, call ffb_update_filters_for_type or ffb_set_samplerate to
 * rebuild coefficients on live effects. */
void ffb_set_filter_preset(ffb_lib_t* lib, uint8_t profile,
                           const ffb_effect_filter_preset_t* preset);
void ffb_get_filter_preset(ffb_lib_t* lib, uint8_t profile,
                           ffb_effect_filter_preset_t* out);

/* Rebuild biquad coefficients on every live effect of the given type
 * (effect-type codes from ffb_defs.h, e.g. FFB_EFFECT_DAMPER). */
void ffb_update_filters_for_type(ffb_lib_t* lib, uint8_t effect_type);

/* HID descriptors (static; can be called without ffb_create). */
const uint8_t* ffb_descriptor_1axis(uint16_t* out_len);
const uint8_t* ffb_descriptor_2axis(uint16_t* out_len);

#ifdef __cplusplus
}
#endif

#endif /* FFB_C_H_ */
