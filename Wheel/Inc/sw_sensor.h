/*
 * sw_magnetometer.h
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#ifndef SRC_SW_MAGNETOMETER_H_
#define SRC_SW_MAGNETOMETER_H_

#include "common_types.h"
#include "hw_magnetometer.h"

typedef struct {
	Magnetometer_HandleTypeDef *hw_magnetometer;
} Sensor_ConfigHandleTypeDef;

typedef struct _Sensor_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Sensor_HandleTypeDef *sensor, Sensor_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Sensor_HandleTypeDef *sensor);
	Wheel_Status (*Update)(struct _Sensor_HandleTypeDef *sensor);
	Wheel_Status (*GetAxis)(struct _Sensor_HandleTypeDef *sensor);

	int16_t virtual_axis;
	uint16_t physical_axis;
	uint16_t previous_sensor_capture;
	uint16_t current_sensor_capture;
	int32_t steering_pos;
	int8_t magnet_full_rotation_cnt;
	int32_t min;
	int32_t max;
	uint8_t start_settling_cnt;
	uint16_t distance;
	float axis_scale;

	Magnetometer_HandleTypeDef *hw_magnetometer;
} Sensor_HandleTypeDef;

#endif /* SRC_SW_MAGNETOMETER_H_ */
