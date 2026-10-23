/*
 * wheel_file_settings.c
 *
 *  Created on: Oct 20, 2026
 *      Author: raffi
 *
 *  Ranges of the settings files, see wheel_file_settings.h. The modules that
 *  read the settings image rely on them: they never check a value again
 */

#include "wheel_file_types.h"
#include "wheel_log.h"

#include <stddef.h>

// A cutoff from WHEEL_FILE_SETTINGS_MIN_FILTER_HZ to _MAX_FILTER_HZ
static bool filter_hz_valid(uint16_t hz) {
  return hz >= WHEEL_FILE_SETTINGS_MIN_FILTER_HZ
      && hz <= WHEEL_FILE_SETTINGS_MAX_FILTER_HZ;
}

// True when every byte of a reserved field is 0
static bool reserved_clear(const uint8_t *bytes, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (bytes[i] != 0u) {
      return false;
    }
  }
  return true;
}

// The intensities take any value. A Q of 0 would divide by zero in the filters
static bool ffb_valid(const wheel_file_settings_ffb_t *ffb) {
  return ffb->strength_pct <= WHEEL_FILE_SETTINGS_MAX_STRENGTH_PCT
      && ffb->constant_filter_q >= WHEEL_FILE_SETTINGS_MIN_FILTER_Q
      && ffb->reserved == 0u && filter_hz_valid(ffb->constant_filter_hz)
      && filter_hz_valid(ffb->damper_filter_hz)
      && filter_hz_valid(ffb->friction_filter_hz)
      && filter_hz_valid(ffb->inertia_filter_hz);
}

static bool steering_valid(const wheel_file_settings_steering_t *steering) {
  return steering->endstop_margin_deg <= WHEEL_FILE_SETTINGS_MAX_ENDSTOP_MARGIN_DEG
      && filter_hz_valid(steering->speed_filter_hz)
      && filter_hz_valid(steering->accel_filter_hz)
      && steering->speed_filter_q >= WHEEL_FILE_SETTINGS_MIN_FILTER_Q
      && steering->accel_filter_q >= WHEEL_FILE_SETTINGS_MIN_FILTER_Q;
}

// A threshold or a tolerance of 0 could keep the calibration from ever ending.
// The recenter compares the tolerance with an int16_t
static bool calibration_valid(const wheel_file_settings_calibration_t *calibration) {
  return calibration->force >= 1u
      && calibration->force <= WHEEL_FILE_SETTINGS_MAX_CALIBRATION_FORCE
      && calibration->stall_threshold >= 1u && calibration->min_range >= 1u
      && calibration->recenter_tolerance >= 1u
      && calibration->recenter_tolerance <= INT16_MAX;
}

static bool shifter_valid(const wheel_file_settings_shifter_t *shifter) {
  return shifter->min.x < shifter->max.x && shifter->min.y > shifter->max.y;
}

// Equal ends would divide by zero
static bool pedals_valid(const wheel_file_settings_pedals_t *pedals) {
  for (size_t i = 0; i < 3u; i++) {
    if (pedals->released[i] == pedals->pressed[i]) {
      return false;
    }
  }
  return true;
}

// An odd vote that fits in the sample buffer
static bool buttons_valid(const wheel_file_settings_buttons_t *buttons) {
  uint8_t vote = buttons->vote_samples;
  return vote >= 1u && vote <= WHEEL_FILE_SETTINGS_MAX_VOTE_SAMPLES
         && (vote % 2u) == 1u && buttons->reserved == 0u;
}

static bool system_valid(const wheel_file_settings_system_t *system) {
  return system->log_level <= WHEEL_LOG_LEVEL
      && reserved_clear(system->reserved, sizeof(system->reserved));
}

// No force asked, no force given, and a stronger force never gives less
static bool force_valid(const wheel_file_settings_force_t *force) {
  if (force->lut[0] != 0u) {
    return false;
  }
  for (size_t i = 1; i < sizeof(force->lut); i++) {
    if (force->lut[i] < force->lut[i - 1u]) {
      return false;
    }
  }
  return true;
}

// The check of each type. Any other type is not a settings type: false
bool wheel_file_settings_valid(uint8_t type, const void *content) {
  switch (type) {
  case WHEEL_FILE_SETTINGS_FFB:
    return ffb_valid(content);
  case WHEEL_FILE_SETTINGS_STEERING:
    return steering_valid(content);
  case WHEEL_FILE_SETTINGS_CALIBRATION:
    return calibration_valid(content);
  case WHEEL_FILE_SETTINGS_SHIFTER:
    return shifter_valid(content);
  case WHEEL_FILE_SETTINGS_PEDALS:
    return pedals_valid(content);
  case WHEEL_FILE_SETTINGS_BUTTONS:
    return buttons_valid(content);
  case WHEEL_FILE_SETTINGS_SYSTEM:
    return system_valid(content);
  case WHEEL_FILE_SETTINGS_FORCE:
    return force_valid(content);
  default:
    return false;
  }
}
