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
 * sw_actuator.h
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#ifndef COMMON_TEMPLATES_SW_ACTUATOR_H_
#define COMMON_TEMPLATES_SW_ACTUATOR_H_

#include "hw_motor_driver.h"
#include "common_types.h"

#define MOTOR_MAX_FORCE 255
#define MOTOR_MIN_FORCE -255

typedef struct _Actuator_ConfigHandleTypeDef {
	MotorDriver_HandleTypeDef *hMotorDriver;
} Actuator_ConfigHandleTypeDef;

typedef struct _Actuator_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Actuator_HandleTypeDef *hActuator,
			Actuator_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Actuator_HandleTypeDef *hActuator);
	Wheel_Status (*Apply_Force)(struct _Actuator_HandleTypeDef *hActuator,
			int16_t force); // full range -> [-255;+255]

	MotorDriver_HandleTypeDef *hMotorDriver;
} Actuator_HandleTypeDef;

#endif /* COMMON_TEMPLATES_SW_ACTUATOR_H_ */
