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

Actuator_HandleTypeDef hActuator = {Actuator_INIT, Actuator_DeINIT,
    Actuator_Apply_Force};

static Wheel_Status Actuator_INIT(Actuator_HandleTypeDef *hActuator,
                                  Actuator_ConfigHandleTypeDef *config) {
  if (config == NULL) {
    return WHEEL_ERROR;
  }
  if (config->hMotorDriver == NULL || config->settings == NULL) {
    return WHEEL_ERROR;
  }
  memcpy(&hActuator->Config, config, sizeof(Actuator_ConfigHandleTypeDef));
  return WHEEL_OK;
}

// Also deinitializes the motor driver, which releases the motor: this module
// is its only user. Safe on a module that is not initialized
static Wheel_Status Actuator_DeINIT(Actuator_HandleTypeDef *hActuator) {
  Actuator_ConfigHandleTypeDef *config = &hActuator->Config;
  Wheel_Status ret = WHEEL_OK;
  if (config->hMotorDriver != NULL) {
    ret = config->hMotorDriver->DeINIT(config->hMotorDriver);
  }
  memset(&hActuator->Config, 0, sizeof(Actuator_ConfigHandleTypeDef));
  return ret;
}

static Wheel_Status Actuator_Apply_Force(Actuator_HandleTypeDef *hActuator,
                                         int16_t force) {
  MotorDriver_HandleTypeDef *motor = hActuator->Config.hMotorDriver;
  // lut[force asked] is the force given to the motor driver
  const uint8_t *lut = hActuator->Config.settings->lut;
  Wheel_Status ret = WHEEL_OK;
  uint8_t corrected_force = 0;
  // Out of range means that something went badly wrong upstream: apply no
  // force rather than a wrong one
  if (force > MOTOR_MAX_FORCE || force < MOTOR_MIN_FORCE) {
    WHEEL_LOG_ERROR(WHEEL_ERR_MOTOR);
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
    // The drivers stay enabled: Coast() is not used
  }
  return ret;
}
