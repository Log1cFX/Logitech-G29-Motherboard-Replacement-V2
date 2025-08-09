/*
 * buttons.c
 *
 *  Created on: Jul 26, 2025
 *      Author: raffi
 */

#include "sw_buttons.h"

static Wheel_Status Buttons_INIT(Buttons_HandleTypeDef *buttons,
		Buttons_ConfigHandleTypeDef *config);
static Wheel_Status Buttons_DeINIT(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_Start_TIM(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_Stop(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_Update(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_GetState(Buttons_HandleTypeDef *buttons);

static void read_rotary_buttons_state(Buttons_HandleTypeDef *buttons);
static void get_debounced_state(Buttons_HandleTypeDef *buttons);
//static void getRotaryButtons(Buttons_SW_HandleTypeDef *buttons);

Buttons_HandleTypeDef hButtons = { Buttons_INIT, Buttons_DeINIT,
		Buttons_Start_TIM, Buttons_Stop, Buttons_Update, Buttons_GetState };

static Wheel_Status Buttons_INIT(Buttons_HandleTypeDef *buttons,
		Buttons_ConfigHandleTypeDef *config) {
	if (config->htim == 0 || config->hw_buttons == 0) {
		return WHEEL_ERROR;
	}
	buttons->htim = config->htim;
	buttons->hw_buttons = config->hw_buttons;
	return WHEEL_OK;
}
static Wheel_Status Buttons_DeINIT(Buttons_HandleTypeDef *buttons) {
	if (buttons->hw_buttons->DeINIT(buttons->hw_buttons) == WHEEL_ERROR) {
		return WHEEL_ERROR;
	}
	buttons->htim = 0;
	buttons->hw_buttons = 0;
	return WHEEL_OK;
}

static Wheel_Status Buttons_Start_TIM(Buttons_HandleTypeDef *buttons) {
	HAL_TIM_Base_Start_IT(buttons->htim);
	return WHEEL_OK;
}

static Wheel_Status Buttons_Stop(Buttons_HandleTypeDef *buttons) {
	HAL_TIM_Base_Stop_IT(buttons->htim);
	return WHEEL_ERROR;
}

static Wheel_Status Buttons_Update(Buttons_HandleTypeDef *buttons) {
	buttons->sample_head = (buttons->sample_head + 1) % BUTTONS_BUFFER_SIZE;
	buttons->sample_buffer[buttons->sample_head] =
			buttons->hw_buttons->buttons_state;
	read_rotary_buttons_state(buttons);
	return WHEEL_OK;
}

static Wheel_Status Buttons_GetState(Buttons_HandleTypeDef *buttons) {
	get_debounced_state(buttons);
	return WHEEL_OK;
}

static void get_debounced_state(Buttons_HandleTypeDef *buttons) {
	uint16_t buttonSum[BUTTONS_NUM] = { 0 };
	uint32_t *buffer = buttons->sample_buffer;
	uint16_t head = buttons->sample_head;
	uint16_t sample_idx;

	for (uint16_t offset = 0; offset < MAX_SAMPLES; offset++) {
		// Go only through the latest samples
		sample_idx =
				(head + BUTTONS_BUFFER_SIZE - offset) % BUTTONS_BUFFER_SIZE;
		for (uint8_t button = 0; button < BUTTONS_NUM; button++) {
			buttonSum[button] += (buffer[sample_idx] >> button) & 0X01;
		}
	}
	buttons->buttons_state = 0;
	uint8_t isActive;
	for (int8_t button = BUTTONS_NUM - 1; button >= 0; button--) {
		isActive = (buttonSum[button] > HALF_SAMPLES) ? 1 : 0;
		buttons->buttons_state = (buttons->buttons_state << 1) | isActive;
	}
}

static void read_rotary_buttons_state(Buttons_HandleTypeDef *buttons) {
	uint32_t *buffer = buttons->sample_buffer;
	uint32_t *sequence = buttons->knob_rotation_sequence;
	uint8_t *rot_idx = &buttons->knob_head;
	uint16_t sample_idx = buttons->sample_head;

	if (GET_BIT(buffer[sample_idx], RL_KNOB_BIT_MASK) != sequence[*rot_idx]) {
		*rot_idx = (*rot_idx + 1) % ROTATION_SEQUENCE_SIZE;
		sequence[*rot_idx] = GET_BIT(buffer[sample_idx], RL_KNOB_BIT_MASK);
	}
}

//static void getRotaryButtons(Buttons_SW_HandleTypeDef *buttons) {
//
//}
