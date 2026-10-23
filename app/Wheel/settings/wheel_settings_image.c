/*
 * wheel_settings_image.c
 *
 *  Created on: Oct 20, 2026
 *      Author: raffi
 *
 *  The RAM image of the settings, and the values the firmware is flashed with.
 *  The linker puts the bytes of the initializer in the settings page (.settings
 *  in the linker script), checksum.py fills in the crc of each file, and
 *  wheel_settings_init() copies the page back here. Never const: what the image
 *  holds changes, the compiler must read it every time
 */

#include "wheel_settings.h"
#include "wheel_log.h"

// The header of a settings file. Its crc is filled in by checksum.py
#define HEADER(type, content_t) {type, WHEEL_VERSION, sizeof(content_t), 0u}

// @formatter:off
__attribute__((section(".settings")))
wheel_settings_page_t wheel_settings_image = {
  .ffb = {
    .header = HEADER(WHEEL_FILE_SETTINGS_FFB, wheel_file_settings_ffb_t),
    .content = {
      .endstop_strength = 250,
      .idle_spring_strength = 255,
      .damper_intensity = 0,
      .friction_intensity = 0,
      .inertia_intensity = 0,
      .strength_pct = 100,
      // Library defaults: 500 Hz and Q 0.70, which filter nothing at 1 kHz
      .constant_filter_q = 60,
      .constant_filter_hz = 50,
      .damper_filter_hz = 15, // library default 30
      .friction_filter_hz = 15, // library default 50
      .inertia_filter_hz = 5, // library default 15
    },
  },
  .steering = {
    .header = HEADER(WHEEL_FILE_SETTINGS_STEERING, wheel_file_settings_steering_t),
    // A lower cutoff smooths the derivatives of the position more, for more
    // lag. Library defaults: 70 Hz Q 0.55 (speed), 55 Hz Q 0.30 (acceleration)
    .content = {
      .endstop_margin_deg = 15,
      .speed_filter_hz = 40,
      .accel_filter_hz = 25,
      .speed_filter_q = 55,
      .accel_filter_q = 30,
    },
  },
  .calibration = {
    .header = HEADER(WHEEL_FILE_SETTINGS_CALIBRATION,
                     wheel_file_settings_calibration_t),
    .content = {
      .force = 135,
      .stall_threshold = 150,
      .min_range = 63750, // about 64069 was measured in testing
      .recenter_tolerance = 150,
    },
  },
  .shifter = {
    .header = HEADER(WHEEL_FILE_SETTINGS_SHIFTER, wheel_file_settings_shifter_t),
    .content = {
      .min = {.x = 10000, .y = 60000},
      .max = {.x = 52000, .y = 0},
    },
  },
  .pedals = {
    .header = HEADER(WHEEL_FILE_SETTINGS_PEDALS, wheel_file_settings_pedals_t),
    // The full ADC range, inverted: a released pedal reads high
    .content = {
      .released = {0xFFFF, 0xFFFF, 0xFFFF},
      .pressed = {0, 0, 0},
    },
  },
  .buttons = {
    .header = HEADER(WHEEL_FILE_SETTINGS_BUTTONS, wheel_file_settings_buttons_t),
    .content = {
      .vote_samples = 65,
      .knob_lock_ms = 30,
    },
  },
  .system = {
    .header = HEADER(WHEEL_FILE_SETTINGS_SYSTEM, wheel_file_settings_system_t),
    .content = {
      .log_level = WHEEL_LOG_LEVEL,
    },
  },
  .force = {
    .header = HEADER(WHEEL_FILE_SETTINGS_FORCE, wheel_file_settings_force_t),
    .content = {
      .lut = {0, 37, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 43, 43, 43,
        43, 44, 44, 44, 45, 45, 45, 46, 46, 47, 47, 48, 48, 49, 50, 51, 52, 53,
        54, 54, 55, 56, 56, 57, 58, 59, 59, 60, 61, 62, 63, 64, 64, 65, 66, 67,
        68, 69, 70, 71, 72, 73, 74, 75, 75, 75, 76, 77, 78, 79, 80, 80, 81, 82,
        83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 98, 99,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 112,
        114, 115, 116, 117, 118, 119, 120, 125, 126, 126, 127, 127, 128, 128,
        128, 128, 129, 131, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139,
        141, 142, 142, 143, 144, 145, 147, 148, 149, 149, 150, 151, 152, 153,
        154, 155, 160, 160, 160, 161, 161, 161, 161, 162, 164, 166, 166, 167,
        168, 169, 170, 171, 172, 173, 174, 176, 177, 177, 178, 179, 180, 181,
        183, 184, 184, 185, 186, 186, 188, 188, 190, 191, 192, 193, 194, 195,
        196, 197, 198, 198, 199, 199, 200, 201, 203, 204, 204, 205, 206, 206,
        207, 208, 209, 210, 210, 211, 211, 211, 212, 213, 213, 214, 214, 215,
        215, 215, 216, 216, 217, 218, 218, 219, 220, 220, 221, 221, 222, 222,
        223, 224, 224, 224, 224, 225, 225, 225, 226, 226, 227, 227, 227, 228,
        228, 230, 230, 231, 231, 231, 232, 232, 233, 233, 234, 235, 238, 240,
        255},
    },
  },
};
// @formatter:on
