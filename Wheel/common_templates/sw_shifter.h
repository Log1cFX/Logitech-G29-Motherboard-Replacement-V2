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
 * sw_shifter.h
 *
 *  Created on: Aug 14, 2025
 *      Author: raffi
 */

#ifndef COMMON_TEMPLATES_SW_SHIFTER_H_
#define COMMON_TEMPLATES_SW_SHIFTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "hw_analog_input.h"

typedef struct {
	uint16_t x;
	uint16_t y;
}Point;

typedef struct {
	Analog_HandleTypeDef *hw_analog;
	uint16_t modifier_pin;
	GPIO_TypeDef *modifier_port;
}Shifter_ConfigHandleTypeDef;

typedef struct _Shifter_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Shifter_HandleTypeDef *shifter,
			Shifter_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Shifter_HandleTypeDef *shifter);
	Wheel_Status (*GetState)(struct _Shifter_HandleTypeDef *shifter); // call this before reading gear

	uint16_t modifier_pin;
	GPIO_TypeDef *modifier_port;

	Point min;
	Point max;
	Point current_pos;
	uint8_t gear;

	Analog_HandleTypeDef *hw_analog;
}Shifter_HandleTypeDef;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TEMPLATES_SW_SHIFTER_H_ */
