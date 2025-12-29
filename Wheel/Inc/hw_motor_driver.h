/*
 * hw_motor_driver.h
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#ifndef INC_HW_MOTOR_DRIVER_H_
#define INC_HW_MOTOR_DRIVER_H_

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

#endif /* INC_HW_MOTOR_DRIVER_H_ */
