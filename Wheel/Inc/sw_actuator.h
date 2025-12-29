/*
 * sw_actuator.h
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#ifndef INC_SW_ACTUATOR_H_
#define INC_SW_ACTUATOR_H_

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

#endif /* INC_SW_ACTUATOR_H_ */
