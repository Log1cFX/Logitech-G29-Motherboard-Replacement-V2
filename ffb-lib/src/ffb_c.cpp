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
 * ffb_c.cpp
 *
 * C wrapper implementation. A single static ffb::Library lives here,
 * constructed in place by ffb_create() (no heap).
 */

#include "ffb/ffb.h"
#include "ffb/ffb_c.h"

#include <new>

namespace {

alignas(ffb::Library) unsigned char g_storage[sizeof(ffb::Library)];
bool g_created = false;
ffb::Library* g_lib_ptr = nullptr;

inline ffb::Library* as_lib(ffb_lib_t* h) {
    return reinterpret_cast<ffb::Library*>(h);
}

} /* anonymous namespace */

extern "C" {

ffb_lib_t* ffb_create(uint8_t axis_count,
                      ffb_time_fn_t millis_fn,
                      ffb_time_fn_t micros_fn) {
    if (g_created) {
        return reinterpret_cast<ffb_lib_t*>(g_lib_ptr);
    }
    ffb::TimeSource ts{};
    ts.millis = millis_fn;
    ts.micros = micros_fn;
    g_lib_ptr = new (g_storage) ffb::Library(axis_count, ts);
    g_created = true;
    return reinterpret_cast<ffb_lib_t*>(g_lib_ptr);
}

void ffb_set_send_report_callback(ffb_lib_t* lib, ffb_send_report_fn_t cb) {
    if (!lib) return;
    as_lib(lib)->setSendReportCallback(cb);
}

void ffb_hid_out(ffb_lib_t* lib, uint8_t report_id,
                 const uint8_t* buf, uint16_t len) {
    if (!lib) return;
    as_lib(lib)->hidOut(report_id, buf, len);
}

uint16_t ffb_hid_get(ffb_lib_t* lib, uint8_t report_id,
                     uint8_t* reply, uint16_t reqlen) {
    if (!lib) return 0;
    return as_lib(lib)->hidGet(report_id, reply, reqlen);
}

void ffb_set_axis_state(ffb_lib_t* lib, uint8_t axis,
                        int32_t pos_scaled_16b, float speed, float accel) {
    if (!lib) return;
    ffb::AxisState s;
    s.pos_scaled_16b = pos_scaled_16b;
    s.speed          = speed;
    s.accel          = accel;
    as_lib(lib)->setAxisState(axis, s);
}

void ffb_calculate(ffb_lib_t* lib) {
    if (!lib) return;
    as_lib(lib)->calculate();
}

int32_t ffb_get_axis_torque(ffb_lib_t* lib, uint8_t axis) {
    if (!lib) return 0;
    return as_lib(lib)->getAxisTorque(axis);
}

void ffb_set_active(ffb_lib_t* lib, bool on) {
    if (!lib) return;
    as_lib(lib)->setActive(on);
}

bool ffb_is_active(ffb_lib_t* lib) {
    if (!lib) return false;
    return as_lib(lib)->isActive();
}

void ffb_reset_all_effects(ffb_lib_t* lib) {
    if (!lib) return;
    as_lib(lib)->resetAllEffects();
}

void ffb_set_global_gain(ffb_lib_t* lib, uint8_t gain) {
    if (!lib) return;
    as_lib(lib)->setGlobalGain(gain);
}

void ffb_set_samplerate(ffb_lib_t* lib, float hz) {
    if (!lib) return;
    as_lib(lib)->setSamplerate(hz);
}

void ffb_set_direction_enable_mask(ffb_lib_t* lib, uint8_t mask) {
    if (!lib) return;
    as_lib(lib)->setDirectionEnableMask(mask);
}

void ffb_set_axis_state_s(ffb_lib_t* lib, uint8_t axis,
                          const ffb_axis_state_t* state) {
    if (!lib || !state) return;
    ffb::AxisState s;
    s.pos_scaled_16b = state->pos_scaled_16b;
    s.speed          = state->speed;
    s.accel          = state->accel;
    as_lib(lib)->setAxisState(axis, s);
}

uint8_t ffb_get_global_gain(ffb_lib_t* lib) {
    if (!lib) return 0;
    return as_lib(lib)->getGlobalGain();
}

float ffb_get_samplerate(ffb_lib_t* lib) {
    if (!lib) return 0.0f;
    return as_lib(lib)->getSamplerate();
}

uint8_t ffb_get_axis_count(ffb_lib_t* lib) {
    if (!lib) return 0;
    return as_lib(lib)->getCalculator().getAxisCount();
}

void ffb_set_friction_rampup_pct(ffb_lib_t* lib, uint8_t pct) {
    if (!lib) return;
    as_lib(lib)->getCalculator().setFrictionRampupPct(pct);
}

uint8_t ffb_get_friction_rampup_pct(ffb_lib_t* lib) {
    if (!lib) return 0;
    return as_lib(lib)->getCalculator().getFrictionRampupPct();
}

void ffb_set_filter_profile_id(ffb_lib_t* lib, uint8_t id) {
    if (!lib) return;
    as_lib(lib)->getCalculator().setFilterProfileId(id);
}

uint8_t ffb_get_filter_profile_id(ffb_lib_t* lib) {
    if (!lib) return 0;
    return as_lib(lib)->getCalculator().getFilterProfileId();
}

void ffb_set_effect_gains(ffb_lib_t* lib, const ffb_effect_gain_t* gains) {
    if (!lib || !gains) return;
    ffb::EffectGain& g = as_lib(lib)->getCalculator().gains();
    g.spring   = gains->spring;
    g.damper   = gains->damper;
    g.inertia  = gains->inertia;
    g.friction = gains->friction;
}

void ffb_get_effect_gains(ffb_lib_t* lib, ffb_effect_gain_t* out) {
    if (!lib || !out) return;
    ffb::EffectGain& g = as_lib(lib)->getCalculator().gains();
    out->spring   = g.spring;
    out->damper   = g.damper;
    out->inertia  = g.inertia;
    out->friction = g.friction;
}

void ffb_set_effect_scalers(ffb_lib_t* lib, const ffb_effect_scaler_t* scalers) {
    if (!lib || !scalers) return;
    ffb::EffectScaler& s = as_lib(lib)->getCalculator().scalers();
    s.spring   = scalers->spring;
    s.damper   = scalers->damper;
    s.inertia  = scalers->inertia;
    s.friction = scalers->friction;
}

void ffb_get_effect_scalers(ffb_lib_t* lib, ffb_effect_scaler_t* out) {
    if (!lib || !out) return;
    ffb::EffectScaler& s = as_lib(lib)->getCalculator().scalers();
    out->spring   = s.spring;
    out->damper   = s.damper;
    out->inertia  = s.inertia;
    out->friction = s.friction;
}

void ffb_set_filter_preset(ffb_lib_t* lib, uint8_t profile,
                           const ffb_effect_filter_preset_t* preset) {
    if (!lib || !preset) return;
    ffb::EffectFilterPreset& p = as_lib(lib)->getCalculator().filterPreset(profile);
    p.constant.freq = preset->constant_freq; p.constant.q = preset->constant_q;
    p.friction.freq = preset->friction_freq; p.friction.q = preset->friction_q;
    p.damper.freq   = preset->damper_freq;   p.damper.q   = preset->damper_q;
    p.inertia.freq  = preset->inertia_freq;  p.inertia.q  = preset->inertia_q;
}

void ffb_get_filter_preset(ffb_lib_t* lib, uint8_t profile,
                           ffb_effect_filter_preset_t* out) {
    if (!lib || !out) return;
    ffb::EffectFilterPreset& p = as_lib(lib)->getCalculator().filterPreset(profile);
    out->constant_freq = p.constant.freq; out->constant_q = p.constant.q;
    out->friction_freq = p.friction.freq; out->friction_q = p.friction.q;
    out->damper_freq   = p.damper.freq;   out->damper_q   = p.damper.q;
    out->inertia_freq  = p.inertia.freq;  out->inertia_q  = p.inertia.q;
}

void ffb_update_filters_for_type(ffb_lib_t* lib, uint8_t effect_type) {
    if (!lib) return;
    as_lib(lib)->getCalculator().updateFiltersForType(effect_type);
}

const uint8_t* ffb_descriptor_1axis(uint16_t* out_len) {
    return ffb::descriptor1Axis(out_len);
}

const uint8_t* ffb_descriptor_2axis(uint16_t* out_len) {
    return ffb::descriptor2Axis(out_len);
}

} /* extern "C" */
