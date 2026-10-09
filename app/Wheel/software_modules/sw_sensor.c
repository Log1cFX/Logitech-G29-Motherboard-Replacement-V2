/*
 * sw_sensor.c
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#include "sw_sensor.h"

// The capture is 16 bits and wraps once per turn of the magnet
#define SENSOR_CAPTURE_MAX 65535.0f
#define SENSOR_COUNTS_PER_REV 65536
// A bigger jump between two captures means that the capture wrapped
#define SENSOR_HALF_REV 32767
// Captures ignored at startup, while the previous capture is not valid yet
#define SENSOR_SETTLING_SAMPLES 2

static Wheel_Status Sensor_INIT(Sensor_HandleTypeDef *sensor,
                                Sensor_ConfigHandleTypeDef *config);
static Wheel_Status Sensor_DeINIT(Sensor_HandleTypeDef *sensor);
static Wheel_Status Sensor_Update(Sensor_HandleTypeDef *sensor);
static Wheel_Status Sensor_GetAxis(Sensor_HandleTypeDef *sensor);

Sensor_HandleTypeDef hSensor = {Sensor_INIT, Sensor_DeINIT, Sensor_Update,
    Sensor_GetAxis};

static void reset_state(Sensor_HandleTypeDef *sensor);
static inline void calculate_magnet_rotations(Sensor_HandleTypeDef *sensor);
static inline void get_steering_pos(Sensor_HandleTypeDef *sensor);

static Wheel_Status Sensor_INIT(Sensor_HandleTypeDef *sensor,
                                Sensor_ConfigHandleTypeDef *config) {
  if (config == NULL) {
    return WHEEL_ERROR;
  }
  if (config->hw_magnetometer == NULL) {
    return WHEEL_ERROR;
  }
  memcpy(&sensor->Config, config, sizeof(Sensor_ConfigHandleTypeDef));
  // A calibration starts from min and max: those of a previous use must go
  reset_state(sensor);
  sensor->start_settling_cnt = SENSOR_SETTLING_SAMPLES;
  sensor->axis_scale = 1.0f;
  return WHEEL_OK;
}

// Also deinitializes the magnetometer, which stops the poll: this module is
// its only user. Safe on a module that is not initialized
static Wheel_Status Sensor_DeINIT(Sensor_HandleTypeDef *sensor) {
  Magnetometer_HandleTypeDef *hw_magnetometer = sensor->Config.hw_magnetometer;
  Wheel_Status ret = WHEEL_OK;
  // Magnetometer first: once it is stopped, Update() no longer runs and the
  // values below can be cleared
  if (hw_magnetometer != NULL) {
    ret |= hw_magnetometer->DeINIT(hw_magnetometer);
  }
  reset_state(sensor);
  memset(&sensor->Config, 0, sizeof(Sensor_ConfigHandleTypeDef));
  return ret;
}

// Only call when the magnetometer is not polled
static void reset_state(Sensor_HandleTypeDef *sensor) {
  sensor->virtual_axis = 0;
  sensor->steering_pos = 0;
  sensor->min = 0;
  sensor->max = 0;
  sensor->physical_axis = 0;
  sensor->previous_sensor_capture = 0;
  sensor->current_sensor_capture = 0;
  sensor->magnet_full_rotation_cnt = 0;
  sensor->start_settling_cnt = 0;
  sensor->distance = 0;
  sensor->axis_scale = 0.0f;
}

static Wheel_Status Sensor_Update(Sensor_HandleTypeDef *sensor) {
  sensor->previous_sensor_capture = sensor->current_sensor_capture;
  sensor->current_sensor_capture = sensor->Config.hw_magnetometer->reading;
  if (sensor->start_settling_cnt == 0) {
    calculate_magnet_rotations(sensor);
  } else {
    sensor->start_settling_cnt--;
  }
  return WHEEL_OK;
}

// Maps steering_pos from [min, max] to the full int16 range, clamped at both ends
static Wheel_Status Sensor_GetAxis(Sensor_HandleTypeDef *sensor) {
  get_steering_pos(sensor);
  uint16_t distance = sensor->max - sensor->min;
  sensor->distance = distance;

  uint16_t physical = sensor->steering_pos - sensor->min;
  sensor->physical_axis = physical;

  uint16_t half = distance / 2U;
  int16_t virtual_offset = (int16_t)physical - (int16_t)half;

  if (virtual_offset > (int16_t)half) {
    virtual_offset = (int16_t)half;
  }
  if (virtual_offset < -(int16_t)half) {
    virtual_offset = -(int16_t)half;
  }
  float scaled = virtual_offset * sensor->axis_scale;
  if (scaled > INT16_MAX) {
    scaled = INT16_MAX;
  } else if (scaled < INT16_MIN) {
    scaled = INT16_MIN;
  }
  sensor->virtual_axis = (int16_t)scaled;

  return WHEEL_OK;
}

// steering_pos units per turn of the magnet
static const float roll_to_axis_coef = 1560.3571f;

// Counts the turns of the magnet: a jump of more than half a turn between two
// captures is a wrap
static inline void calculate_magnet_rotations(Sensor_HandleTypeDef *sensor) {
  uint16_t cur = sensor->current_sensor_capture;
  uint16_t prev = sensor->previous_sensor_capture;

  int32_t diff = (int32_t)cur - (int32_t)prev;

  if (diff > SENSOR_HALF_REV) {
    diff -= SENSOR_COUNTS_PER_REV;
    sensor->magnet_full_rotation_cnt--;
  } else if (diff < -SENSOR_HALF_REV) {
    diff += SENSOR_COUNTS_PER_REV;
    sensor->magnet_full_rotation_cnt++;
  }
}

static inline void get_steering_pos(Sensor_HandleTypeDef *sensor) {
  float fraction = (float)sensor->current_sensor_capture / SENSOR_CAPTURE_MAX;
  float pos = sensor->magnet_full_rotation_cnt * roll_to_axis_coef
      + fraction * roll_to_axis_coef;
  sensor->steering_pos = (int32_t)pos;
}

