/*
 * sw_actuator.c
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#include "sw_actuator.h"

static Wheel_Status Actuator_INIT(Actuator_HandleTypeDef *hActuator,
		Actuator_ConfigHandleTypeDef *config);
static Wheel_Status Actuator_DeINIT(Actuator_HandleTypeDef *hActuator);
static Wheel_Status Actuator_Apply_Force(Actuator_HandleTypeDef *hActuator,
		int16_t force);

Actuator_HandleTypeDef hActuator = { Actuator_INIT, Actuator_DeINIT,
		Actuator_Apply_Force };

static Wheel_Status Actuator_INIT(Actuator_HandleTypeDef *hActuator,
		Actuator_ConfigHandleTypeDef *config) {
	if (config == NULL) {
		return WHEEL_ERROR;
	}
	if (config->hMotorDriver == NULL) {
		return WHEEL_ERROR;
	}
	hActuator->hMotorDriver = config->hMotorDriver;
	return WHEEL_OK;
}

static Wheel_Status Actuator_DeINIT(Actuator_HandleTypeDef *hActuator) {

	if (hActuator->hMotorDriver->DeINIT(hActuator->hMotorDriver)
			== WHEEL_ERROR) {
		return WHEEL_ERROR;
	}
	hActuator = NULL;
	return WHEEL_OK;
}

static Wheel_Status Actuator_Apply_Force(Actuator_HandleTypeDef *hActuator,
		int16_t force) {
	MotorDriver_HandleTypeDef *motor = hActuator->hMotorDriver;
	Wheel_Status ret = WHEEL_OK; // just to confirm that nothing has gone wrong
	/*
	 * force is set to 0 because if it is more than 255 it means that something went critically wrong
	 * in the steps before and if that is the case then we don't want these forces to get to the user
	 */
	if (force > MOTOR_MAX_FORCE) {
		force = 0;
		ret = WHEEL_ERROR;
	} else if (force < MOTOR_MIN_FORCE) {
		force = 0;
		ret = WHEEL_ERROR;
	}
	if (force < 0) {
		ret != motor->Drive_Left(motor, -force);
	} else if (force > 0) {
		ret != motor->Drive_Right(motor, force);
	} else {
		ret != motor->Coast(motor);
	}
	return ret;
}
