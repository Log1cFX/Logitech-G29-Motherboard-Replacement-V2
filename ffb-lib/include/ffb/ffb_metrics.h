/*
 * ffb_metrics.h  (optional)
 *
 * Helper that derives filtered speed and acceleration from a raw position
 * stream. Equivalent to OpenFFBoard's Axis::updateMetrics() math:
 *
 *     speed = (new_pos - prev_pos) * samplerate
 *     speed = speedFilter.process(speed)
 *     accel = (speed - last_speed) * samplerate
 *     accel = accelFilter.process(accel)
 *
 * Users who already compute filtered speed/accel themselves can ignore
 * this header entirely and feed AxisState directly to Library::setAxisState().
 */

#ifndef FFB_METRICS_H_
#define FFB_METRICS_H_

#include <cstdint>

#include "ffb/ffb_biquad.h"
#include "ffb/ffb_calculator.h"

namespace ffb {

/* Default filter coefficients - copied from Axis::filterSpeedCst /
 * filterAccelCst at profile index 1 (medium). */
struct MetricsFilterPreset {
    biquad_constant_t speed = { 70, 55 };
    biquad_constant_t accel = { 55, 30 };
};

class MetricsBuilder {
public:
    MetricsBuilder(float degrees_of_rotation,
                   float samplerate_hz       = FFB_DEFAULT_SAMPLERATE_HZ,
                   MetricsFilterPreset preset = MetricsFilterPreset{});

    /* Push a new raw position (in degrees) and return the resulting
     * AxisState that you can feed straight into Library::setAxisState(). */
    AxisState update(float new_pos_degrees);

    /* Re-derive filter coefficients (call after changing the FFB rate). */
    void setSamplerate(float hz);

    /* Re-init at a given position (clears speed/accel history). */
    void reset(float pos_degrees);

private:
    /* Convert raw degrees into a -0x7fff..0x7fff scaled int. The full
     * range maps to +/- (degrees_of_rotation / 2). */
    int32_t scalePos(float pos_degrees) const;

    float degrees_of_rotation;
    float samplerate;
    float last_pos = 0;
    float last_speed_raw = 0;
    Biquad speed_filter;
    Biquad accel_filter;
    MetricsFilterPreset preset;
};

} /* namespace ffb */

#endif /* FFB_METRICS_H_ */
