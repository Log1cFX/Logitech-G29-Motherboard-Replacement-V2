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
 * hw_analog_input.h
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#ifndef COMMON_TEMPLATES_HW_ANALOG_INPUT_H_
#define COMMON_TEMPLATES_HW_ANALOG_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

#define ANALOG_INPUT_NUM 5 // number of analog inputs
#define PEDALS_NUM 3 // number of pedals
#define PEDALS_IDX 0 // index inside the axis array for the first pedal
#define SHIFTER_IDX 3 // index inside the axis array for the shifter

typedef struct {
	ADC_HandleTypeDef *hadc;
} Analog_ConfigHandleTypeDef;

typedef struct _Analog_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Analog_HandleTypeDef *analog,
			Analog_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Analog_HandleTypeDef *analog);
	Wheel_Status (*Start_CONTINUOUS_SCAN_DMA)(
			struct _Analog_HandleTypeDef *analog); // look up "CONTINUOUS DMA SCAN STM32"
	Wheel_Status (*Stop)(struct _Analog_HandleTypeDef *analog);

	uint32_t axis[ANALOG_INPUT_NUM]; // array where output will be stored

	ADC_HandleTypeDef *hadc;
} Analog_HandleTypeDef;

typedef struct _Pedals_ConfigHandleTypeDef {
	Analog_HandleTypeDef *hw_analog;
} Pedals_ConfigHandleTypeDef;

typedef struct _Pedals_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Pedals_HandleTypeDef *analog,
			Pedals_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Pedals_HandleTypeDef *analog);
	Wheel_Status (*GetState)(struct _Pedals_HandleTypeDef *analog);

	uint8_t clutch;
	uint8_t brake;
	uint8_t throtle;

	Analog_HandleTypeDef *hw_analog;
} Pedals_HandleTypeDef;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TEMPLATES_HW_ANALOG_INPUT_H_ */
