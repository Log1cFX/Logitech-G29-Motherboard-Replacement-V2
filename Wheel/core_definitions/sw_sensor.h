/*
 MIT License

 Copyright (c) 2025 Log1cFX

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/*
 * sw_sensor.h
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#ifndef CORE_DEFINITIONS_SW_SENSOR_H_
#define CORE_DEFINITIONS_SW_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "hw_magnetometer.h"

typedef struct {
	Magnetometer_HandleTypeDef *hw_magnetometer;
}Sensor_ConfigHandleTypeDef;

typedef struct _Sensor_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Sensor_HandleTypeDef *sensor, Sensor_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Sensor_HandleTypeDef *sensor);
	Wheel_Status (*Update)(struct _Sensor_HandleTypeDef *sensor);
	Wheel_Status (*GetAxis)(struct _Sensor_HandleTypeDef *sensor);

	int16_t virtual_axis; // the actual value that's sent, using full range
	uint16_t physical_axis;// essential for calculation logic, uses full range
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
}Sensor_HandleTypeDef;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_SW_SENSOR_H_ */
