/*
 * buttons.c
 *
 *  Created on: Jul 26, 2025
 *      Author: raffi
 */

#include "sw_buttons.h"
#include "string.h"

static Wheel_Status Buttons_INIT(Buttons_HandleTypeDef *buttons,
		Buttons_ConfigHandleTypeDef *config);
static Wheel_Status Buttons_DeINIT(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_Start_TIM(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_Stop(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_Update(Buttons_HandleTypeDef *buttons);
static Wheel_Status Buttons_GetState(Buttons_HandleTypeDef *buttons);

static void get_debounced_state(Buttons_HandleTypeDef *buttons);
static void read_knob_button_state(Buttons_HandleTypeDef *buttons);
static void update_knob_button_state(Buttons_HandleTypeDef *buttons);
static void get_current_knob_state(Buttons_HandleTypeDef *buttons);

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
	return WHEEL_OK;
}

static Wheel_Status Buttons_Update(Buttons_HandleTypeDef *buttons) {
	buttons->sample_head = (buttons->sample_head + 1) % BUTTONS_BUFFER_SIZE;
	buttons->sample_buffer[buttons->sample_head] =
			buttons->hw_buttons->buttons_state;
	if (GET_BIT(buttons->knob_flags, KNOB_LOCK_FLAG)) {
		update_knob_button_state(buttons);
		__NOP();
	} else {
		read_knob_button_state(buttons);
	}
	return WHEEL_OK;
}

static Wheel_Status Buttons_GetState(Buttons_HandleTypeDef *buttons) {
	get_debounced_state(buttons);
	get_current_knob_state(buttons);
	return WHEEL_OK;
}

static void get_debounced_state(Buttons_HandleTypeDef *buttons) {
	uint16_t buttonSum[BUTTONS_NUM] = { 0 };
	uint32_t *buffer = buttons->sample_buffer;
	uint16_t head = buttons->sample_head;
	uint16_t sample_idx = head;

	for (uint16_t offset = 0; offset < MAX_SAMPLES; offset++) {
		if (sample_idx == 0) {
			sample_idx = BUTTONS_BUFFER_SIZE - 1;
		} else {
			sample_idx--;
		}
		uint32_t sample = buffer[sample_idx];
		buttonSum[0] += (sample >> 0) & 1;
		buttonSum[1] += (sample >> 1) & 1;
		buttonSum[2] += (sample >> 2) & 1;
		buttonSum[3] += (sample >> 3) & 1;
		buttonSum[4] += (sample >> 4) & 1;
		buttonSum[5] += (sample >> 5) & 1;
		buttonSum[6] += (sample >> 6) & 1;
		buttonSum[7] += (sample >> 7) & 1;
		buttonSum[8] += (sample >> 8) & 1;
		buttonSum[9] += (sample >> 9) & 1;
		buttonSum[10] += (sample >> 10) & 1;
		buttonSum[11] += (sample >> 11) & 1;
		buttonSum[12] += (sample >> 12) & 1;
		buttonSum[13] += (sample >> 13) & 1;
		buttonSum[14] += (sample >> 14) & 1;
		buttonSum[15] += (sample >> 15) & 1;
		buttonSum[16] += (sample >> 16) & 1;
		buttonSum[17] += (sample >> 17) & 1;
		buttonSum[18] += (sample >> 18) & 1;
		buttonSum[19] += (sample >> 19) & 1;
		buttonSum[20] += (sample >> 20) & 1;
		buttonSum[21] += (sample >> 21) & 1;
		buttonSum[22] += (sample >> 22) & 1;
		buttonSum[23] += (sample >> 23) & 1;
	}

	uint32_t result = 0;
	for (uint8_t button = 0; button < BUTTONS_NUM; button++) {
		if (buttonSum[button] > HALF_SAMPLES) {
			result |= (1UL << button);
		}
	}
	buttons->buttons_state = result;
}

static void get_current_knob_state(Buttons_HandleTypeDef *buttons) {
	CLEAR_BIT(buttons->buttons_state, RL_KNOB_BIT_MASK);
	if (GET_BIT(buttons->knob_flags, KNOB_LOCK_FLAG)) {
		if(GET_BIT(buttons->knob_flags, KNOB_DIRECTION_FLAG)){
			SET_BIT(buttons->buttons_state, R_KNOB_BIT_MASK);
		} else {
			SET_BIT(buttons->buttons_state, L_KNOB_BIT_MASK);
		}
	}
}

const uint32_t right_sequence_backwards[ROTATION_SEQUENCE_SIZE] = { 0,
L_KNOB_BIT_MASK, RL_KNOB_BIT_MASK, R_KNOB_BIT_MASK, 0 };
const uint32_t left_sequence_backwards[ROTATION_SEQUENCE_SIZE] = { 0,
R_KNOB_BIT_MASK, RL_KNOB_BIT_MASK, L_KNOB_BIT_MASK, 0 };

static void read_knob_button_state(Buttons_HandleTypeDef *buttons) {
	uint32_t *buffer = buttons->sample_buffer;
	uint32_t *sequence = buttons->knob_rotation_sequence_buffer;
	uint8_t knob_idx = buttons->knob_head;
	uint16_t sample_idx = buttons->sample_head;

	if (GET_BIT(buffer[sample_idx], RL_KNOB_BIT_MASK) != sequence[knob_idx]) {
		knob_idx = (knob_idx + 1) % ROTATION_SEQUENCE_SIZE;
		sequence[knob_idx] = GET_BIT(buffer[sample_idx], RL_KNOB_BIT_MASK);
		buttons->knob_head = knob_idx;
	} else {
		return;
	}
	uint8_t found_sequence = 0;
	const uint32_t *sequences[] = { left_sequence_backwards,
			right_sequence_backwards };
	uint8_t seq;

	for (seq = 0; seq < 2; seq++) {
		uint8_t idx = knob_idx;
		for (uint8_t offset = 0; offset < ROTATION_SEQUENCE_SIZE; offset++) {
			// Walk backwards through circular buffer
			idx = (knob_idx + ROTATION_SEQUENCE_SIZE - offset)
					% ROTATION_SEQUENCE_SIZE;
			if (sequence[idx] != sequences[seq][offset]) {
				break;
			}
			if (offset == ROTATION_SEQUENCE_SIZE - 1)
				found_sequence = 1;
		}
		if (found_sequence) {
			break;
		}
	}
	if (found_sequence) {
		SET_BIT(buttons->knob_flags, KNOB_LOCK_FLAG);
		if (seq == 1) {
			SET_BIT(buttons->knob_flags, KNOB_DIRECTION_FLAG);
		}
		buttons->knob_lock_init_time_ms = HAL_GetTick();
	}
}

static void update_knob_button_state(Buttons_HandleTypeDef *buttons) {
	if (HAL_GetTick() - buttons->knob_lock_init_time_ms > KNOB_LOCK_TIME_MS) {
		CLEAR_BIT(buttons->knob_flags, KNOB_LOCK_FLAG);
		CLEAR_BIT(buttons->knob_flags, KNOB_DIRECTION_FLAG);
		memset(buttons->knob_rotation_sequence_buffer, 0,
				sizeof(ROTATION_SEQUENCE_SIZE));
	}
}
