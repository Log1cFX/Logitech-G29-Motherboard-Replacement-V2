/*
 * ffb_axis_local_c.cpp
 *
 * C wrapper for ffb::AxisLocalEffects. Instances live in a static pool of
 * FFB_MAX_AXIS slots (no heap), constructed in place by
 * ffb_axis_local_create().
 */

#include "ffb/ffb_axis_local_c.h"

#include "ffb/ffb_axis_local.h"

#include <new>

namespace {

alignas(ffb::AxisLocalEffects) unsigned char g_storage[FFB_MAX_AXIS][sizeof(ffb::AxisLocalEffects)];
unsigned g_count = 0;

inline ffb::AxisLocalEffects* as_a(ffb_axis_local_t *h) {
	return reinterpret_cast<ffb::AxisLocalEffects*>(h);
}

} /* anonymous namespace */

extern "C" {

void ffb_axis_local_config_default(ffb_axis_local_config_t *out) {
	if (!out)
		return;
	ffb::AxisLocalConfig d; /* C++ defaults */
	out->idle_spring_strength = d.idle_spring_strength;
	out->endstop_strength = d.endstop_strength;
	out->damper_intensity = d.damper_intensity;
	out->friction_intensity = d.friction_intensity;
	out->inertia_intensity = d.inertia_intensity;
	out->degrees_of_rotation = d.degrees_of_rotation;
	out->damper_filter_freq = d.damper_filter.freq;
	out->damper_filter_q = d.damper_filter.q;
	out->friction_filter_freq = d.friction_filter.freq;
	out->friction_filter_q = d.friction_filter.q;
	out->inertia_filter_freq = d.inertia_filter.freq;
	out->inertia_filter_q = d.inertia_filter.q;
	out->samplerate_hz = d.samplerate_hz;
}

ffb_axis_local_t* ffb_axis_local_create(const ffb_axis_local_config_t *cfg) {
	if (g_count >= FFB_MAX_AXIS) {
		return nullptr;
	}
	ffb::AxisLocalConfig c; /* starts at C++ defaults */
	if (cfg) {
		c.idle_spring_strength = cfg->idle_spring_strength;
		c.endstop_strength = cfg->endstop_strength;
		c.damper_intensity = cfg->damper_intensity;
		c.friction_intensity = cfg->friction_intensity;
		c.inertia_intensity = cfg->inertia_intensity;
		c.degrees_of_rotation = cfg->degrees_of_rotation;
		c.damper_filter.freq = cfg->damper_filter_freq;
		c.damper_filter.q = cfg->damper_filter_q;
		c.friction_filter.freq = cfg->friction_filter_freq;
		c.friction_filter.q = cfg->friction_filter_q;
		c.inertia_filter.freq = cfg->inertia_filter_freq;
		c.inertia_filter.q = cfg->inertia_filter_q;
		c.samplerate_hz = cfg->samplerate_hz;
	}
	void *slot = g_storage[g_count++];
	ffb::AxisLocalEffects *a = new (slot) ffb::AxisLocalEffects(c);
	return reinterpret_cast<ffb_axis_local_t*>(a);
}

int32_t ffb_axis_local_compute(ffb_axis_local_t *a,
		const ffb_axis_state_t *metrics, float pos_degrees, bool ffb_on) {
	if (!a || !metrics)
		return 0;
	ffb::AxisState s;
	s.pos_scaled_16b = metrics->pos_scaled_16b;
	s.speed = metrics->speed;
	s.accel = metrics->accel;
	return as_a(a)->compute(s, pos_degrees, ffb_on);
}

void ffb_axis_local_set_samplerate(ffb_axis_local_t *a, float hz) {
	if (!a)
		return;
	as_a(a)->setSamplerate(hz);
}

void ffb_axis_local_set_idle_spring(ffb_axis_local_t *a, uint8_t strength) {
	if (!a)
		return;
	as_a(a)->setIdleSpringStrength(strength);
}

void ffb_axis_local_set_intensities(ffb_axis_local_t *a,
		uint8_t endstop_strength, uint8_t damper_intensity,
		uint8_t friction_intensity, uint8_t inertia_intensity) {
	if (!a)
		return;
	ffb::AxisLocalConfig &c = as_a(a)->config();
	c.endstop_strength = endstop_strength;
	c.damper_intensity = damper_intensity;
	c.friction_intensity = friction_intensity;
	c.inertia_intensity = inertia_intensity;
}

} /* extern "C" */
