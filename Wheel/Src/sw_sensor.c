/*
 * sw_magnetometer.c
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#include "sw_sensor.h"

static Wheel_Status Sensor_SW_INIT(Sensor_HandleTypeDef *sensor,
		Sensor_ConfigHandleTypeDef *config);
static Wheel_Status Sensor_SW_DeINIT(Sensor_HandleTypeDef *sensor);
static Wheel_Status Sensor_SW_Update(Sensor_HandleTypeDef *sensor);
static Wheel_Status Sensor_SW_GetAxis(Sensor_HandleTypeDef *sensor);

Sensor_HandleTypeDef hSensor = { Sensor_SW_INIT, Sensor_SW_DeINIT,
		Sensor_SW_Update, Sensor_SW_GetAxis };

static inline void calculate_magnet_rotations(Sensor_HandleTypeDef *sensor);
static inline void get_steering_pos(Sensor_HandleTypeDef *sensor);

static Wheel_Status Sensor_SW_INIT(Sensor_HandleTypeDef *sensor,
		Sensor_ConfigHandleTypeDef *config) {
	if (config->hw_magnetometer == 0) {
		return WHEEL_ERROR;
	}
	sensor->hw_magnetometer = config->hw_magnetometer;
	sensor->start_settling_cnt = 2;
	return WHEEL_OK;
}

static Wheel_Status Sensor_SW_DeINIT(Sensor_HandleTypeDef *sensor) {
	if (sensor->hw_magnetometer->DeINIT(sensor->hw_magnetometer)
			== WHEEL_ERROR) {
		return WHEEL_ERROR;
	}
	sensor->hw_magnetometer = 0;
	return WHEEL_OK;
}

static Wheel_Status Sensor_SW_Update(Sensor_HandleTypeDef *sensor) {
	sensor->previous_sensor_capture = sensor->current_sensor_capture;
	sensor->current_sensor_capture = (int16_t) sensor->hw_magnetometer->alpha;
	if (sensor->start_settling_cnt == 0) {
		calculate_magnet_rotations(sensor);
	} else {
		sensor->start_settling_cnt--;
	}
	return WHEEL_OK;
}

static Wheel_Status Sensor_SW_GetAxis(Sensor_HandleTypeDef *sensor) {
	get_steering_pos(sensor);
	sensor->distance = (uint16_t) (sensor->max - sensor->min);
	sensor->physical_axis = (uint16_t) (sensor->steering_pos - sensor->min);
	sensor->virtual_axis = sensor->physical_axis - (sensor->distance / 2);
	return WHEEL_OK;
}

/* Conversion coefficient: one full magnetic revolution → roll_to_axis units */
static const float roll_to_axis_coef = 1560.3571f;

/*
 * Calculate full revolutions by comparing current vs previous 16-bit capture,
 * using half-range threshold (32767) for unwrap logic.
 */
static inline void calculate_magnet_rotations(Sensor_HandleTypeDef *sensor) {
	uint16_t cur = sensor->current_sensor_capture;
	uint16_t prev = sensor->previous_sensor_capture;

	int32_t diff = (int32_t) cur - (int32_t) prev;

	if (diff > 32767) {
		diff -= 65536;
		sensor->magnet_full_rotation_cnt--;
	} else if (diff < -32767) {
		diff += 65536;
		sensor->magnet_full_rotation_cnt++;
	}
}

static inline void get_steering_pos(Sensor_HandleTypeDef *sensor) {
	float fraction = (float) sensor->current_sensor_capture / 65535.0f;
	float pos = sensor->magnet_full_rotation_cnt * roll_to_axis_coef
			+ fraction * roll_to_axis_coef;
	sensor->steering_pos = (int32_t) pos;
}

