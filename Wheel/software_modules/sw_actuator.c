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

// force correction generated with wheelcheck and lut generator
const uint8_t lut[256] = { 0, 37, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 43,
		43, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 47, 47, 48, 48, 49, 50, 51,
		52, 53, 54, 54, 55, 56, 56, 57, 58, 59, 59, 60, 61, 62, 63, 64, 64, 65,
		66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75, 75, 76, 77, 78, 79, 80, 80,
		81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
		98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
		112, 114, 115, 116, 117, 118, 119, 120, 125, 126, 126, 127, 127, 128,
		128, 128, 128, 129, 131, 131, 132, 133, 134, 135, 135, 136, 137, 138,
		139, 141, 142, 142, 143, 144, 145, 147, 148, 149, 149, 150, 151, 152,
		153, 154, 155, 160, 160, 160, 161, 161, 161, 161, 162, 164, 166, 166,
		167, 168, 169, 170, 171, 172, 173, 174, 176, 177, 177, 178, 179, 180,
		181, 183, 184, 184, 185, 186, 186, 188, 188, 190, 191, 192, 193, 194,
		195, 196, 197, 198, 198, 199, 199, 200, 201, 203, 204, 204, 205, 206,
		206, 207, 208, 209, 210, 210, 211, 211, 211, 212, 213, 213, 214, 214,
		215, 215, 215, 216, 216, 217, 218, 218, 219, 220, 220, 221, 221, 222,
		222, 223, 224, 224, 224, 224, 225, 225, 225, 226, 226, 227, 227, 227,
		228, 228, 230, 230, 231, 231, 231, 232, 232, 233, 233, 234, 235, 238,
		240, 255 };

static Wheel_Status Actuator_INIT(Actuator_HandleTypeDef *hActuator,
		Actuator_ConfigHandleTypeDef *config) {
	if (config == NULL) {
		return WHEEL_ERROR;
	}
	if (config->hMotorDriver == NULL) {
		return WHEEL_ERROR;
	}
	memcpy(&hActuator->Config, config, sizeof(Actuator_ConfigHandleTypeDef));
	return WHEEL_OK;
}

static Wheel_Status Actuator_DeINIT(Actuator_HandleTypeDef *hActuator) {
	Actuator_ConfigHandleTypeDef *config = &hActuator->Config;
	if (config->hMotorDriver->DeINIT(config->hMotorDriver) == WHEEL_ERROR) {
		return WHEEL_ERROR;
	}
	hActuator = NULL;
	return WHEEL_OK;
}

static Wheel_Status Actuator_Apply_Force(Actuator_HandleTypeDef *hActuator,
		int16_t force) {
	MotorDriver_HandleTypeDef *motor = hActuator->Config.hMotorDriver;
	Wheel_Status ret = WHEEL_OK; // just to confirm that nothing has gone wrong
	uint8_t corrected_force = 0;
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
		corrected_force = lut[-force];
		ret |= motor->Drive_Left(motor, corrected_force);
	} else if (force > 0) {
		corrected_force = lut[force];
		ret |= motor->Drive_Right(motor, corrected_force);
	} else {
		motor->Drive_Right(motor, 0);
		motor->Drive_Left(motor, 0);
//		ret |= motor->Coast(motor);
	}
	return ret;
}
