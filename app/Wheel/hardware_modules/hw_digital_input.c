/*
 * hw_digital_input.c
 *
 *  Created on: Aug 1, 2025
 *      Author: raffi
 */

#include "hw_digital_input.h"

static Wheel_Status DigitalInput_INIT(DigitalInput_HandleTypeDef *buttons,
                                      DigitalInput_ConfigHandleTypeDef *config);
static Wheel_Status DigitalInput_DeINIT(DigitalInput_HandleTypeDef *buttons);
static Wheel_Status DigitalInput_ReadState(DigitalInput_HandleTypeDef *buttons);

DigitalInput_HandleTypeDef hG29Buttons = {DigitalInput_INIT, DigitalInput_DeINIT,
    DigitalInput_ReadState};

static Wheel_Status DigitalInput_INIT(DigitalInput_HandleTypeDef *buttons,
                                      DigitalInput_ConfigHandleTypeDef *config) {
  if (config == NULL) {
    return WHEEL_ERROR;
  }
  if (config->buttons_port == NULL || config->clk_pin == 0 || config->in_pin == 0
      || config->lock_pin == 0) {
    return WHEEL_ERROR;
  }
  memcpy(&buttons->Config, config, sizeof(DigitalInput_ConfigHandleTypeDef));
  buttons->buttons_state = 0;
  return WHEEL_OK;
}

// Nothing to stop here. ReadState() must not be called afterwards: sw_buttons
// stops its poll timer before it calls this
static Wheel_Status DigitalInput_DeINIT(DigitalInput_HandleTypeDef *buttons) {
  buttons->buttons_state = 0;
  memset(&buttons->Config, 0, sizeof(DigitalInput_ConfigHandleTypeDef));
  return WHEEL_OK;
}

// Locks the state of the buttons, then reads one button per clock pulse
static Wheel_Status DigitalInput_ReadState(DigitalInput_HandleTypeDef *buttons) {
  buttons->buttons_state = 0;
  DigitalInput_ConfigHandleTypeDef *config = &buttons->Config;
  HAL_GPIO_WritePin(config->buttons_port, config->clk_pin, 0);
  HAL_GPIO_WritePin(config->buttons_port, config->lock_pin, 1);
  for (uint8_t i = 0; i < BUTTONS_NUM; i++) {
    HAL_GPIO_WritePin(config->buttons_port, config->clk_pin, 0);
    if (HAL_GPIO_ReadPin(config->buttons_port, config->in_pin)) {
      SET_BIT(buttons->buttons_state, (1U << i));
    }
    HAL_GPIO_WritePin(config->buttons_port, config->clk_pin, 1);
  }
  HAL_GPIO_WritePin(config->buttons_port, config->lock_pin, 0);
  return WHEEL_OK;
}
