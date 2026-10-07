/*
 * sw_magnetometer.c
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#include "sw_sensor.h"

/* The capture is a 16 bit value that wraps around once per revolution of the magnet */
#define SENSOR_CAPTURE_MAX 65535.0f // highest capture value
#define SENSOR_COUNTS_PER_REV 65536 // number of capture values in one revolution
// a jump bigger than this between two captures means that the capture wrapped around
#define SENSOR_HALF_REV 32767
// number of captures ignored at startup before counting revolutions,
// because the previous capture isn't valid yet
#define SENSOR_SETTLING_SAMPLES 2

static Wheel_Status Sensor_INIT(Sensor_HandleTypeDef *sensor,
		Sensor_ConfigHandleTypeDef *config);
static Wheel_Status Sensor_DeINIT(Sensor_HandleTypeDef *sensor);
static Wheel_Status Sensor_Update(Sensor_HandleTypeDef *sensor);
static Wheel_Status Sensor_GetAxis(Sensor_HandleTypeDef *sensor);

Sensor_HandleTypeDef hSensor = { Sensor_INIT, Sensor_DeINIT, Sensor_Update,
		Sensor_GetAxis };

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
	// the calibration of a previous use must not be kept : it starts from
	// min and max to find the new ends
	reset_state(sensor);
	sensor->start_settling_cnt = SENSOR_SETTLING_SAMPLES;
	sensor->axis_scale = 1.0f;
	return WHEEL_OK;
}

// Deinitializes the magnetometer (this module is the only one that uses it),
// which stops the poll, and forgets everything, the calibration included.
// Can be called on a module that is not initialized, it does nothing then
static Wheel_Status Sensor_DeINIT(Sensor_HandleTypeDef *sensor) {
	Magnetometer_HandleTypeDef *hw_magnetometer = sensor->Config.hw_magnetometer;
	Wheel_Status ret = WHEEL_OK;
	// the magnetometer goes first : once it is stopped, Update() isn't called
	// anymore and the values below can be cleared
	if (hw_magnetometer != NULL) {
		ret |= hw_magnetometer->DeINIT(hw_magnetometer);
	}
	reset_state(sensor);
	memset(&sensor->Config, 0, sizeof(Sensor_ConfigHandleTypeDef));
	return ret;
}

// puts back everything the module computes, like after a power on.
// Only to call when the magnetometer isn't polled
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

static Wheel_Status Sensor_GetAxis(Sensor_HandleTypeDef *sensor) {
	get_steering_pos(sensor);
	uint16_t distance = sensor->max - sensor->min;
	sensor->distance = distance;

	uint16_t physical = sensor->steering_pos - sensor->min;
	sensor->physical_axis = physical;

	uint16_t half = distance / 2U;
	int16_t virtual_offset = (int16_t) physical - (int16_t) half;

	if (virtual_offset > (int16_t) half) {
		virtual_offset = (int16_t) half;
	}
	if (virtual_offset < -(int16_t) half) {
		virtual_offset = -(int16_t) half;
	}
	float scaled = virtual_offset * sensor->axis_scale;
	if (scaled > INT16_MAX) {
		scaled = INT16_MAX;
	} else if (scaled < INT16_MIN) {
		scaled = INT16_MIN;
	}
	sensor->virtual_axis = (int16_t) scaled;

	return WHEEL_OK;
}

/* Conversion coefficient: one full magnetic revolution -> roll_to_axis units */
static const float roll_to_axis_coef = 1560.3571f;

// Calculate full revolutions by comparing current vs previous 16-bit capture,
// using half-range threshold (SENSOR_HALF_REV) for unwrap logic.
static inline void calculate_magnet_rotations(Sensor_HandleTypeDef *sensor) {
	uint16_t cur = sensor->current_sensor_capture;
	uint16_t prev = sensor->previous_sensor_capture;

	int32_t diff = (int32_t) cur - (int32_t) prev;

	if (diff > SENSOR_HALF_REV) {
		diff -= SENSOR_COUNTS_PER_REV;
		sensor->magnet_full_rotation_cnt--;
	} else if (diff < -SENSOR_HALF_REV) {
		diff += SENSOR_COUNTS_PER_REV;
		sensor->magnet_full_rotation_cnt++;
	}
}

static inline void get_steering_pos(Sensor_HandleTypeDef *sensor) {
	float fraction = (float) sensor->current_sensor_capture / SENSOR_CAPTURE_MAX;
	float pos = sensor->magnet_full_rotation_cnt * roll_to_axis_coef
			+ fraction * roll_to_axis_coef;
	sensor->steering_pos = (int32_t) pos;
}

