/*
 MIT License

 Copyright (c) 2025 Log1cFX

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/*
 * hw_digital_input.h
 *
 *  Created on: Aug 1, 2025
 *      Author: raffi
 */

#ifndef COMMON_TEMPLATES_HW_DIGITAL_INPUT_H_
#define COMMON_TEMPLATES_HW_DIGITAL_INPUT_H_

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

#endif /* COMMON_TEMPLATES_HW_DIGITAL_INPUT_H_ */
