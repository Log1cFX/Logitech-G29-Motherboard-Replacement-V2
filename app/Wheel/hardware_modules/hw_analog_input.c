/*
 * hw_analog_input.c
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#include "hw_analog_input.h"

// the ADC gives 16 bit values (12 bit, left aligned), the pedals are sent on 8 bits
#define PEDAL_ADC_TO_8BIT_SHIFT 8
// the pedal axes are inverted : PEDAL_MAX - value
#define PEDAL_MAX 0xFF

static Wheel_Status Analog_INIT(Analog_HandleTypeDef *analog,
		Analog_ConfigHandleTypeDef *config);
static Wheel_Status Analog_DeINIT(Analog_HandleTypeDef *analog);
static Wheel_Status Analog_Start_CONTINIOUS_SCAN_DMA(
		Analog_HandleTypeDef *analog);
static Wheel_Status Analog_Stop(Analog_HandleTypeDef *analog);

Analog_HandleTypeDef hAnalog = { Analog_INIT, Analog_DeINIT,
		Analog_Start_CONTINIOUS_SCAN_DMA, Analog_Stop };

static Wheel_Status Pedals_INIT(Pedals_HandleTypeDef *pedals,
		Pedals_ConfigHandleTypeDef *config);
static Wheel_Status Pedals_DeINIT(Pedals_HandleTypeDef *pedals);
static Wheel_Status Pedals_GetState(Pedals_HandleTypeDef *pedals);

Pedals_HandleTypeDef hPedals = { Pedals_INIT, Pedals_DeINIT, Pedals_GetState };

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

static Wheel_Status Analog_DeINIT(Analog_HandleTypeDef *analog) {
	if (HAL_ADC_Stop_DMA(analog->Config.hadc) == HAL_ERROR) {
		return WHEEL_ERROR;
	}
	for (uint8_t i = 0; i < ANALOG_INPUT_NUM; i++) {
		analog->axis[i] = 0;
	}
	memset(&analog->Config, 0, sizeof(Analog_ConfigHandleTypeDef));
	return WHEEL_OK;
}

static Wheel_Status Analog_Start_CONTINIOUS_SCAN_DMA(
		Analog_HandleTypeDef *analog) {
	Analog_ConfigHandleTypeDef *config = &analog->Config;
	HAL_StatusTypeDef ret = HAL_OK;
	// the cast removes the volatile : HAL only gives the address to the DMA,
	// it never reads the array itself
	ret = HAL_ADC_Start_DMA(config->hadc, (uint32_t*) analog->axis,
			ANALOG_INPUT_NUM);
	return (ret == HAL_OK) ? WHEEL_OK : WHEEL_ERROR;
}

static Wheel_Status Analog_Stop(Analog_HandleTypeDef *analog) {
	HAL_StatusTypeDef ret = HAL_OK;
	HAL_ADC_Stop_DMA(analog->Config.hadc);
	return (ret == HAL_OK) ? WHEEL_OK : WHEEL_ERROR;
}

static Wheel_Status Pedals_INIT(Pedals_HandleTypeDef *pedals,
		Pedals_ConfigHandleTypeDef *config) {
	if (config->hw_analog == 0) {
		return WHEEL_ERROR;
	}
	memcpy(&pedals->Config, config, sizeof(Pedals_ConfigHandleTypeDef));
	return WHEEL_OK;
}

static Wheel_Status Pedals_DeINIT(Pedals_HandleTypeDef *pedals) {
	// hw_analog has to live because the shifter might be using it
	memset(&pedals->Config, 0, sizeof(Pedals_ConfigHandleTypeDef));
	return WHEEL_OK;
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

