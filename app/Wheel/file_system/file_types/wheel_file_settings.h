/*
 * wheel_file_settings.h
 *
 *  Created on: Oct 11, 2026
 *      Author: raffi
 *
 *  Contents of the WHEEL_FILE_SETTINGS_* files (see wheel_file_types.h): one
 *  file per group of settings. They lie together in the settings page, and in
 *  its RAM image (wheel_settings.h).
 *  Each struct is a multiple of 4 bytes, without hidden padding: a reserved
 *  field fills every gap, and is 0. The ranges each value must respect are
 *  checked by wheel_file_settings_valid()
 */

#ifndef FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_SETTINGS_H_
#define FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_SETTINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/* LIMITS */
// Cutoff of every low-pass filter: up to half the 1 kHz rate of the control
// loop. Q is given times 100
#define WHEEL_FILE_SETTINGS_MIN_FILTER_HZ 1u
#define WHEEL_FILE_SETTINGS_MAX_FILTER_HZ 500u
#define WHEEL_FILE_SETTINGS_MIN_FILTER_Q 1u
#define WHEEL_FILE_SETTINGS_MAX_STRENGTH_PCT 100u
#define WHEEL_FILE_SETTINGS_MAX_ENDSTOP_MARGIN_DEG 90u
#define WHEEL_FILE_SETTINGS_MAX_CALIBRATION_FORCE 255u // MOTOR_MAX_FORCE
// Longest vote of the buttons, which also sizes their sample buffer
#define WHEEL_FILE_SETTINGS_MAX_VOTE_SAMPLES 65u

/* FILES */
// Force feedback. Applied as soon as the image changes
typedef struct {
  // Effects computed by the wheel itself, 0 to 255
  uint8_t endstop_strength;
  uint8_t idle_spring_strength;
  uint8_t damper_intensity;
  uint8_t friction_intensity;
  uint8_t inertia_intensity;
  // The whole force, from 0 to 100 % of MOTOR_MAX_FORCE
  uint8_t strength_pct;
  // Low-pass filters of the effects of the host
  uint8_t constant_filter_q;
  uint8_t reserved;
  uint16_t constant_filter_hz;
  uint16_t damper_filter_hz;
  uint16_t friction_filter_hz;
  uint16_t inertia_filter_hz;
} wheel_file_settings_ffb_t;
static_assert(sizeof(wheel_file_settings_ffb_t) == 16,
    "wheel_file_settings_ffb_t is not 16 bytes");

// Steering. Taken when the ffb objects are created: at the next start
typedef struct {
  // Kept before each mechanical end, where the endstop effect starts. The
  // steering axis sent to the host always covers the full travel
  uint16_t endstop_margin_deg;
  // Low-pass filters of the speed and the acceleration of the wheel
  uint16_t speed_filter_hz;
  uint16_t accel_filter_hz;
  uint8_t speed_filter_q;
  uint8_t accel_filter_q;
} wheel_file_settings_steering_t;
static_assert(sizeof(wheel_file_settings_steering_t) == 8,
    "wheel_file_settings_steering_t is not 8 bytes");

// Calibration of the startup
typedef struct {
  uint16_t force; // pushed during the sweeps, out of MOTOR_MAX_FORCE
  // An end is reached when steering_pos moves less than this between two
  // samples
  uint16_t stall_threshold;
  // Smallest end-to-end range accepted, in steering_pos units
  uint16_t min_range;
  uint16_t recenter_tolerance; // centered when |virtual_axis| is under this
} wheel_file_settings_calibration_t;
static_assert(sizeof(wheel_file_settings_calibration_t) == 8,
    "wheel_file_settings_calibration_t is not 8 bytes");

typedef struct {
  uint16_t x;
  uint16_t y;
} wheel_file_settings_point_t;

// Shifter: two opposite corners of the travel, in raw ADC values. y decreases
// from min to max: min.x < max.x and min.y > max.y
typedef struct {
  wheel_file_settings_point_t min;
  wheel_file_settings_point_t max;
} wheel_file_settings_shifter_t;
static_assert(sizeof(wheel_file_settings_shifter_t) == 8,
    "wheel_file_settings_shifter_t is not 8 bytes");

// Pedals, in the order clutch, brake, throttle. A pedal reads 0 at released
// and 255 at pressed, both raw ADC values: their order gives the direction
typedef struct {
  uint16_t released[3];
  uint16_t pressed[3];
} wheel_file_settings_pedals_t;
static_assert(sizeof(wheel_file_settings_pedals_t) == 12,
    "wheel_file_settings_pedals_t is not 12 bytes");

// Buttons
typedef struct {
  // A button is pressed when it is set in more than half of its last
  // vote_samples samples. Odd: a majority without ties
  uint8_t vote_samples;
  uint8_t reserved;
  uint16_t knob_lock_ms; // a detent of the knob is reported this long
} wheel_file_settings_buttons_t;
static_assert(sizeof(wheel_file_settings_buttons_t) == 4,
    "wheel_file_settings_buttons_t is not 4 bytes");

// The firmware itself
typedef struct {
  uint8_t log_level; // see WHEEL_LOG_LEVEL, which it cannot exceed
  uint8_t reserved[3];
} wheel_file_settings_system_t;
static_assert(sizeof(wheel_file_settings_system_t) == 4,
    "wheel_file_settings_system_t is not 4 bytes");

// Force correction of the motor: lut[force asked] is the force given to the
// motor driver, both out of MOTOR_MAX_FORCE. Made with WheelCheck and the LUT
// generator
typedef struct {
  uint8_t lut[256];
} wheel_file_settings_force_t;
static_assert(sizeof(wheel_file_settings_force_t) == 256,
    "wheel_file_settings_force_t is not 256 bytes");

/* CHECK */
// In wheel_file_settings.c. True when every value of a settings content is in
// its range, and every reserved byte is 0. content holds the struct of type,
// a WHEEL_FILE_SETTINGS_* type
bool wheel_file_settings_valid(uint8_t type, const void *content);

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_FILE_TYPES_WHEEL_FILE_SETTINGS_H_ */
