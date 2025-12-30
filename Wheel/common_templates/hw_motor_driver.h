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
 * hw_motor_driver.h
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#ifndef COMMON_TEMPLATES_HW_MOTOR_DRIVER_H_
#define COMMON_TEMPLATES_HW_MOTOR_DRIVER_H_

#include "common_types.h"

typedef struct _MotorDriver_ConfigHandleTypeDef {
	uint16_t R_EN_pin;
	uint16_t L_EN_pin;
	GPIO_TypeDef *R_EN_port;
	GPIO_TypeDef *L_EN_port;
	TIM_HandleTypeDef *pwm_timer;
	uint32_t right_channel;
	uint32_t left_channel;
	volatile uint32_t *right_compareRegister;
	volatile uint32_t *left_compareRegister;
} MotorDriver_ConfigHandleTypeDef;

typedef struct _MotorDriver_HandleTypeDef {
	Wheel_Status (*INIT)(struct _MotorDriver_HandleTypeDef *hMotorDriver,
			MotorDriver_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _MotorDriver_HandleTypeDef *hMotorDriver);
	Wheel_Status (*Drive_Left)(struct _MotorDriver_HandleTypeDef *hMotorDriver,
			uint8_t force); // Counterclockwise; 	full range -> [0;255]
	Wheel_Status (*Drive_Right)(struct _MotorDriver_HandleTypeDef *hMotorDriver,
			uint8_t force); // Clockwise; 		 	full range -> [0;255]
	Wheel_Status (*Coast)(struct _MotorDriver_HandleTypeDef *hMotorDriver);
	uint16_t R_EN_pin;
	uint16_t L_EN_pin;
	GPIO_TypeDef *R_EN_port;
	GPIO_TypeDef *L_EN_port;
	TIM_HandleTypeDef *pwm_timer;
	uint32_t right_channel;
	uint32_t left_channel;
	volatile uint32_t *right_compareRegister;
	volatile uint32_t *left_compareRegister;

	uint8_t motors_enabled;

} MotorDriver_HandleTypeDef;

#endif /* COMMON_TEMPLATES_HW_MOTOR_DRIVER_H_ */
