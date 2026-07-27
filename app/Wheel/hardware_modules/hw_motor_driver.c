/*
 * hw_pwm.c
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#include "hw_motor_driver.h"

static Wheel_Status MotorDriver_INIT(MotorDriver_HandleTypeDef *hMotorDriver,
		MotorDriver_ConfigHandleTypeDef *config);
static Wheel_Status MotorDriver_DeINIT(MotorDriver_HandleTypeDef *hMotorDriver);
static Wheel_Status MotorDriver_Drive_Right(
		MotorDriver_HandleTypeDef *hMotorDriver, uint8_t force);
static Wheel_Status MotorDriver_Drive_Left(
		MotorDriver_HandleTypeDef *hMotorDriver, uint8_t force);
static Wheel_Status MotorDriver_Coast(MotorDriver_HandleTypeDef *hMotorDriver);

MotorDriver_HandleTypeDef hMotorDriver = { MotorDriver_INIT, MotorDriver_DeINIT,
		MotorDriver_Drive_Left, MotorDriver_Drive_Right, MotorDriver_Coast };

static Wheel_Status MotorDriver_INIT(MotorDriver_HandleTypeDef *hMotorDriver,
		MotorDriver_ConfigHandleTypeDef *config) {
	if (config == NULL) {
		return WHEEL_ERROR;
	}
	if ((config->R_EN_pin == 0) || (config->R_EN_pin == 0)
			|| (config->L_EN_port == NULL) || (config->L_EN_port == NULL)
			|| (config->pwm_timer == NULL) || (config->right_channel == 0xFFFF)
			|| (config->left_channel == 0xFFFF)
			|| (config->right_compareRegister == NULL)
			|| (config->left_compareRegister == NULL)) {
		return WHEEL_ERROR;
	}
	memcpy(&hMotorDriver->Config, config,
			sizeof(MotorDriver_ConfigHandleTypeDef));
	uint8_t ret = 0;
	ret |= HAL_TIM_PWM_Start(config->pwm_timer, config->right_channel);
	ret |= HAL_TIM_PWM_Start(config->pwm_timer, config->left_channel);

	return (ret == HAL_OK) ? WHEEL_OK : WHEEL_ERROR;
}

static inline void enable_motors(MotorDriver_HandleTypeDef *hMotorDriver) {
	MotorDriver_ConfigHandleTypeDef *config = &hMotorDriver->Config;
	HAL_GPIO_WritePin(config->R_EN_port, config->R_EN_pin, 1);
	HAL_GPIO_WritePin(config->L_EN_port, config->L_EN_pin, 1);
	hMotorDriver->motors_enabled = 1;
}

static inline void disable_motors(MotorDriver_HandleTypeDef *hMotorDriver) {
	MotorDriver_ConfigHandleTypeDef *config = &hMotorDriver->Config;
	HAL_GPIO_WritePin(config->R_EN_port, config->R_EN_pin, 0);
	HAL_GPIO_WritePin(config->L_EN_port, config->L_EN_pin, 0);
	hMotorDriver->motors_enabled = 0;
}

static Wheel_Status MotorDriver_DeINIT(MotorDriver_HandleTypeDef *hMotorDriver) {
	MotorDriver_ConfigHandleTypeDef *config = &hMotorDriver->Config;
	uint8_t ret = 0;
	ret |= HAL_TIM_PWM_Stop(config->pwm_timer, config->right_channel);
	ret |= HAL_TIM_PWM_Stop(config->pwm_timer, config->left_channel);
	memset(&hMotorDriver->Config, 0, sizeof(MotorDriver_ConfigHandleTypeDef));
	config->right_channel = 0xFFFF;
	config->left_channel = 0xFFFF;
	return (ret == HAL_OK) ? WHEEL_OK : WHEEL_ERROR;
}

static Wheel_Status MotorDriver_Drive_Right(
		MotorDriver_HandleTypeDef *hMotorDriver, uint8_t force) {
	MotorDriver_ConfigHandleTypeDef *config = &hMotorDriver->Config;
	*(config->left_compareRegister) = 0;
	*(config->right_compareRegister) = force;
	if (!hMotorDriver->motors_enabled) {
		enable_motors(hMotorDriver);
	}
	return WHEEL_OK;
}

static Wheel_Status MotorDriver_Drive_Left(
		MotorDriver_HandleTypeDef *hMotorDriver, uint8_t force) {
	MotorDriver_ConfigHandleTypeDef *config = &hMotorDriver->Config;
	*(config->right_compareRegister) = 0;
	*(config->left_compareRegister) = force;
	if (!hMotorDriver->motors_enabled) {
		enable_motors(hMotorDriver);
	}
	return WHEEL_OK;
}

static Wheel_Status MotorDriver_Coast(MotorDriver_HandleTypeDef *hMotorDriver) {
	if (hMotorDriver->motors_enabled) {
		disable_motors(hMotorDriver);
	}
	return WHEEL_OK;
}
