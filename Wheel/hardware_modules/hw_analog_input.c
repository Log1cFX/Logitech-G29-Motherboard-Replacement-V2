/*
 * hw_analog_input.c
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#include "hw_analog_input.h"

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
	if(config == NULL){
		return WHEEL_ERROR;
	}
	if (config->hadc == NULL) {
		return WHEEL_ERROR;
	}
	analog->hadc = config->hadc;
	return WHEEL_OK;
}

static Wheel_Status Analog_DeINIT(Analog_HandleTypeDef *analog) {
	if (HAL_ADC_Stop_DMA(analog->hadc) == HAL_ERROR) {
		return WHEEL_ERROR;
	}
	for (uint8_t i = 0; i < ANALOG_INPUT_NUM; i++) {
		analog->axis[i] = 0;
	}
	analog->hadc = NULL;
	return WHEEL_OK;
}

static Wheel_Status Analog_Start_CONTINIOUS_SCAN_DMA(
		Analog_HandleTypeDef *analog) {
	HAL_StatusTypeDef ret = HAL_OK;
	ret = HAL_ADC_Start_DMA(analog->hadc, analog->axis, ANALOG_INPUT_NUM);
	return (ret==HAL_OK)? WHEEL_OK: WHEEL_ERROR;
}

static Wheel_Status Analog_Stop(Analog_HandleTypeDef *analog) {
	HAL_StatusTypeDef ret = HAL_OK;
	HAL_ADC_Stop_DMA(analog->hadc);
	return (ret==HAL_OK)? WHEEL_OK: WHEEL_ERROR;
}

static Wheel_Status Pedals_INIT(Pedals_HandleTypeDef *pedals,
		Pedals_ConfigHandleTypeDef *config) {
	if (config->hw_analog == 0) {
		return WHEEL_ERROR;
	}
	pedals->hw_analog = config->hw_analog;
	return WHEEL_OK;
}

static Wheel_Status Pedals_DeINIT(Pedals_HandleTypeDef *pedals) {
	// hw_analog has to live because the shifter might be using it
	pedals->hw_analog = NULL;
	return WHEEL_OK;
}

static Wheel_Status Pedals_GetState(Pedals_HandleTypeDef *pedals) {
	pedals->clutch = 0xFF - (pedals->hw_analog->axis[PEDALS_IDX] >> 8);
	pedals->brake = 0xFF - (pedals->hw_analog->axis[PEDALS_IDX+1] >> 8);
	pedals->throtle = 0xFF - (pedals->hw_analog->axis[PEDALS_IDX+2] >> 8);
	if(pedals->hw_analog==NULL){
		return WHEEL_ERROR;
	}
	return WHEEL_OK;
}

