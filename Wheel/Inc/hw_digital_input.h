/*
 * hw_digital_input.h
 *
 *  Created on: Aug 1, 2025
 *      Author: raffi
 */

#ifndef INC_HW_DIGITAL_INPUT_H_
#define INC_HW_DIGITAL_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

#define BUTTONS_NUM 24U
#define MAX_BUTTONS 32U

typedef struct {
	uint16_t clk_pin;
	uint16_t lock_pin;
	uint16_t in_pin;
	GPIO_TypeDef *buttons_port;
} DigitalInput_ConfigHandleTypeDef;

typedef struct _DigitalInput_HandleTypeDef {
	Wheel_Status (*INIT)(struct _DigitalInput_HandleTypeDef *buttons,
			DigitalInput_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _DigitalInput_HandleTypeDef *buttons);
	Wheel_Status (*ReadState)(struct _DigitalInput_HandleTypeDef *buttons);

	uint16_t clk_pin;
	uint16_t lock_pin;
	uint16_t in_pin;
	GPIO_TypeDef *buttons_port;

	uint32_t buttons_state; // the raw state of the read buttons
} DigitalInput_HandleTypeDef;

#ifdef __cplusplus
}
#endif

#endif /* INC_HW_DIGITAL_INPUT_H_ */
