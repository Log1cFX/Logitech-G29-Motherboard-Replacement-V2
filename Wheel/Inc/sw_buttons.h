/*
 * buttons.h
 *
 *  Created on: Jul 26, 2025
 *      Author: raffi
 */

#ifndef INC_SW_BUTTONS_H_
#define INC_SW_BUTTONS_H_

#include "common_types.h"
#include "hw_digital_input.h"

#define MAX_SAMPLES 85U
#define HALF_SAMPLES MAX_SAMPLES/2
#define BUTTONS_BUFFER_SIZE 200U
#define SAMPLE_TIME_MS 1U
#define	KNOB_ROTATION_BLOCK_TIME_MS 8U

#define GET_BIT(var, mask) ((mask) & (var))
#define R_KNOB_BIT_MASK (1U << (23-1))
#define	L_KNOB_BIT_MASK (1U << (24-1))
#define RL_KNOB_BIT_MASK (R_KNOB_BIT_MASK | L_KNOB_BIT_MASK)
#define ROTATION_SEQUENCE_SIZE 5U

typedef struct {
	TIM_HandleTypeDef *htim;
	DigitalInput_HandleTypeDef *hw_buttons;
} Buttons_ConfigHandleTypeDef;

typedef struct _Buttons_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Buttons_HandleTypeDef *buttons,
			Buttons_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Buttons_HandleTypeDef *buttons);
	Wheel_Status (*Start_TIM)(struct _Buttons_HandleTypeDef *buttons);
	Wheel_Status (*Stop)(struct _Buttons_HandleTypeDef *buttons);
	Wheel_Status (*Update)(struct _Buttons_HandleTypeDef *buttons);
	Wheel_Status (*GetState)(struct _Buttons_HandleTypeDef *buttons);

	uint32_t sample_buffer[BUTTONS_BUFFER_SIZE];
	uint16_t sample_head;
	uint32_t knob_rotation_sequence[ROTATION_SEQUENCE_SIZE];
	uint8_t knob_head;
	uint32_t buttons_state;
	uint8_t knob_rotation_flags;

	TIM_HandleTypeDef *htim;
	DigitalInput_HandleTypeDef *hw_buttons;
} Buttons_HandleTypeDef;

#endif /* INC_SW_BUTTONS_H_ */
