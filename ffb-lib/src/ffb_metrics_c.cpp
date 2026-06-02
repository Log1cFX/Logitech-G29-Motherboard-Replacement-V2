/*
 * ffb_metrics_c.cpp
 *
 * C wrapper for ffb::MetricsBuilder. Instances live in a static pool of
 * FFB_MAX_AXIS slots (no heap), constructed in place by ffb_metrics_create().
 */

#include "ffb/ffb_metrics_c.h"

#include "ffb/ffb.h"
#include "ffb/ffb_metrics.h"

#include <new>

namespace {

alignas(ffb::MetricsBuilder)
unsigned char g_storage[FFB_MAX_AXIS][sizeof(ffb::MetricsBuilder)];
unsigned g_count = 0;

inline ffb::MetricsBuilder* as_m(ffb_metrics_t* h) {
    return reinterpret_cast<ffb::MetricsBuilder*>(h);
}

ffb_metrics_t* make(float degrees, float hz, ffb::MetricsFilterPreset preset) {
    if (g_count >= FFB_MAX_AXIS) {
        return nullptr;
    }
    void* slot = g_storage[g_count++];
    ffb::MetricsBuilder* m = new (slot) ffb::MetricsBuilder(degrees, hz, preset);
    return reinterpret_cast<ffb_metrics_t*>(m);
}

} /* anonymous namespace */

extern "C" {

ffb_metrics_t* ffb_metrics_create(float degrees_of_rotation, float samplerate_hz) {
    return make(degrees_of_rotation, samplerate_hz, ffb::MetricsFilterPreset{});
}

ffb_metrics_t* ffb_metrics_create_ex(float degrees_of_rotation, float samplerate_hz,
                                     uint16_t speed_freq, uint8_t speed_q,
                                     uint16_t accel_freq, uint8_t accel_q) {
    ffb::MetricsFilterPreset preset;
    preset.speed.freq = speed_freq; preset.speed.q = speed_q;
    preset.accel.freq = accel_freq; preset.accel.q = accel_q;
    return make(degrees_of_rotation, samplerate_hz, preset);
}

ffb_axis_state_t ffb_metrics_update(ffb_metrics_t* m, float new_pos_degrees) {
    ffb_axis_state_t out = {0, 0.0f, 0.0f};
    if (!m) return out;
    ffb::AxisState s = as_m(m)->update(new_pos_degrees);
    out.pos_scaled_16b = s.pos_scaled_16b;
    out.speed          = s.speed;
    out.accel          = s.accel;
    return out;
}

void ffb_metrics_update_and_set(ffb_metrics_t* m, ffb_lib_t* lib,
                                uint8_t axis, float new_pos_degrees) {
    if (!m || !lib) return;
    ffb_axis_state_t s = ffb_metrics_update(m, new_pos_degrees);
    ffb_set_axis_state_s(lib, axis, &s);
}

void ffb_metrics_set_samplerate(ffb_metrics_t* m, float hz) {
    if (!m) return;
    as_m(m)->setSamplerate(hz);
}

void ffb_metrics_reset(ffb_metrics_t* m, float pos_degrees) {
    if (!m) return;
    as_m(m)->reset(pos_degrees);
}

} /* extern "C" */
