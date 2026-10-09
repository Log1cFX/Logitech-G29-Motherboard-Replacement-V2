/*
 * hw_analog_input.c
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#include "hw_analog_input.h"

// The ADC values are 16 bits (12 bits left-aligned), the pedals are sent on 8
#define PEDAL_ADC_TO_8BIT_SHIFT 8
#define PEDAL_MAX 0xFF // the pedal axes are inverted: PEDAL_MAX - value

static Wheel_Status Analog_INIT(Analog_HandleTypeDef *analog,
                                Analog_ConfigHandleTypeDef *config);
static Wheel_Status Analog_DeINIT(Analog_HandleTypeDef *analog);
static Wheel_Status Analog_Start_CONTINIOUS_SCAN_DMA(Analog_HandleTypeDef *analog);
static Wheel_Status Analog_Stop(Analog_HandleTypeDef *analog);

Analog_HandleTypeDef hAnalog = {Analog_INIT, Analog_DeINIT,
    Analog_Start_CONTINIOUS_SCAN_DMA, Analog_Stop};

static Wheel_Status Pedals_INIT(Pedals_HandleTypeDef *pedals,
                                Pedals_ConfigHandleTypeDef *config);
static Wheel_Status Pedals_DeINIT(Pedals_HandleTypeDef *pedals);
static Wheel_Status Pedals_GetState(Pedals_HandleTypeDef *pedals);
static void reset_pedals(Pedals_HandleTypeDef *pedals);

Pedals_HandleTypeDef hPedals = {Pedals_INIT, Pedals_DeINIT, Pedals_GetState};

static Wheel_Status Analog_INIT(Analog_HandleTypeDef *analog,
                                Analog_ConfigHandleTypeDef *config) {
  if (config == NULL) {
    return WHEEL_ERROR;
  }
  if (config->hadc == NULL) {
    return WHEEL_ERROR;
  }
  memcpy(&analog->Config, config, sizeof(Analog_ConfigHandleTypeDef));
  return WHEEL_OK;
}

// Shared by the pedals and the shifter, which never deinitialize it: call it
// once both are deinitialized. Safe on a module that is not initialized
static Wheel_Status Analog_DeINIT(Analog_HandleTypeDef *analog) {
  Wheel_Status ret = WHEEL_OK;
  // Stop the DMA first: it writes in the array cleared below
  if (analog->Config.hadc != NULL) {
    ret = Analog_Stop(analog);
  }
  for (uint8_t i = 0; i < ANALOG_INPUT_NUM; i++) {
    analog->axis[i] = 0;
  }
  memset(&analog->Config, 0, sizeof(Analog_ConfigHandleTypeDef));
  return ret;
}

static Wheel_Status Analog_Start_CONTINIOUS_SCAN_DMA(Analog_HandleTypeDef *analog) {
  Analog_ConfigHandleTypeDef *config = &analog->Config;
  if (config->hadc == NULL) {
    return WHEEL_ERROR;
  }
  HAL_StatusTypeDef ret = HAL_OK;
  // The cast drops volatile: HAL only hands the address to the DMA
  ret = HAL_ADC_Start_DMA(config->hadc, (uint32_t*)analog->axis,
  ANALOG_INPUT_NUM);
  return (ret == HAL_OK) ? WHEEL_OK : WHEEL_ERROR;
}

static Wheel_Status Analog_Stop(Analog_HandleTypeDef *analog) {
  if (analog->Config.hadc == NULL) {
    return WHEEL_ERROR;
  }
  HAL_StatusTypeDef ret = HAL_OK;
  ret = HAL_ADC_Stop_DMA(analog->Config.hadc);
  return (ret == HAL_OK) ? WHEEL_OK : WHEEL_ERROR;
}

static Wheel_Status Pedals_INIT(Pedals_HandleTypeDef *pedals,
                                Pedals_ConfigHandleTypeDef *config) {
  if (config == NULL) {
    return WHEEL_ERROR;
  }
  if (config->hw_analog == NULL) {
    return WHEEL_ERROR;
  }
  memcpy(&pedals->Config, config, sizeof(Pedals_ConfigHandleTypeDef));
  reset_pedals(pedals);
  return WHEEL_OK;
}

// hw_analog keeps running: the shifter may still use it (see Analog_DeINIT)
static Wheel_Status Pedals_DeINIT(Pedals_HandleTypeDef *pedals) {
  reset_pedals(pedals);
  memset(&pedals->Config, 0, sizeof(Pedals_ConfigHandleTypeDef));
  return WHEEL_OK;
}

static void reset_pedals(Pedals_HandleTypeDef *pedals) {
  pedals->clutch = 0;
  pedals->brake = 0;
  pedals->throtle = 0;
}

static Wheel_Status Pedals_GetState(Pedals_HandleTypeDef *pedals) {
  Analog_HandleTypeDef *hw_analog = pedals->Config.hw_analog;
  pedals->clutch = PEDAL_MAX
      - (hw_analog->axis[PEDALS_IDX] >> PEDAL_ADC_TO_8BIT_SHIFT);
  pedals->brake = PEDAL_MAX
      - (hw_analog->axis[PEDALS_IDX + 1] >> PEDAL_ADC_TO_8BIT_SHIFT);
  pedals->throtle = PEDAL_MAX
      - (hw_analog->axis[PEDALS_IDX + 2] >> PEDAL_ADC_TO_8BIT_SHIFT);
  return WHEEL_OK;
}

