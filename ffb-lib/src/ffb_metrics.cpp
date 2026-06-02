/*
 * ffb_metrics.cpp
 *
 * Speed and acceleration derivation, ported from OpenFFBoard's
 * Axis::updateMetrics().
 */

#include "ffb/ffb_metrics.h"

namespace {
template <typename T>
inline T clip_t(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
} /* anonymous namespace */

namespace ffb {

MetricsBuilder::MetricsBuilder(float degrees, float hz, MetricsFilterPreset p)
    : degrees_of_rotation(degrees > 0.0f ? degrees : 360.0f),
      samplerate(hz > 0.0f ? hz : FFB_DEFAULT_SAMPLERATE_HZ),
      preset(p)
{
    setSamplerate(samplerate);
}

void MetricsBuilder::setSamplerate(float hz) {
    if (hz <= 0.0f) hz = FFB_DEFAULT_SAMPLERATE_HZ;
    samplerate = hz;
    speed_filter.setBiquad(BiquadType::lowpass,
                            preset.speed.freq / samplerate,
                            preset.speed.q / 100.0f, 0.0f);
    accel_filter.setBiquad(BiquadType::lowpass,
                            preset.accel.freq / samplerate,
                            preset.accel.q / 100.0f, 0.0f);
}

void MetricsBuilder::reset(float pos_degrees) {
    last_pos = pos_degrees;
    last_speed_raw = 0;
    /* Reinit filters to clear state. */
    speed_filter.calcBiquad();
    accel_filter.calcBiquad();
}

int32_t MetricsBuilder::scalePos(float pos_degrees) const {
    float half_range = degrees_of_rotation * 0.5f;
    float f = pos_degrees / half_range;            /* -1 .. 1 */
    f = clip_t<float>(f, -1.0f, 1.0f);
    return static_cast<int32_t>(f * 32767.0f);
}

AxisState MetricsBuilder::update(float new_pos_degrees) {
    AxisState out;
    out.pos_scaled_16b = scalePos(new_pos_degrees);

    float speed_raw = (new_pos_degrees - last_pos) * samplerate;
    out.speed = speed_filter.process(speed_raw);

    float accel_raw = (speed_raw - last_speed_raw) * samplerate;
    out.accel = accel_filter.process(accel_raw);

    last_pos = new_pos_degrees;
    last_speed_raw = speed_raw;
    return out;
}

} /* namespace ffb */
