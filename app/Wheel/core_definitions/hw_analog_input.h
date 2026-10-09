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

#ifndef CORE_DEFINITIONS_HW_ANALOG_INPUT_H_
#define CORE_DEFINITIONS_HW_ANALOG_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

// Layout of Analog_HandleTypeDef.axis: the scan order of the ADC, set in main.c
#define ANALOG_INPUT_NUM 5
#define PEDALS_NUM 3
#define PEDALS_IDX 0 // clutch, brake, throttle
#define SHIFTER_IDX 3 // x, y

typedef struct {
  ADC_HandleTypeDef *hadc;
} Analog_ConfigHandleTypeDef;

typedef struct _Analog_HandleTypeDef {
  Wheel_Status (*INIT)(struct _Analog_HandleTypeDef *analog,
                       Analog_ConfigHandleTypeDef *config);
  Wheel_Status (*DeINIT)(struct _Analog_HandleTypeDef *analog);
  Wheel_Status (*Start_CONTINUOUS_SCAN_DMA)(struct _Analog_HandleTypeDef *analog);
  Wheel_Status (*Stop)(struct _Analog_HandleTypeDef *analog);

  Analog_ConfigHandleTypeDef Config;
  // Raw conversions, 12 bits left-aligned in 16. Written by the ADC through DMA
  // at any time, read in SysTick (pedals, shifter)
  volatile uint32_t axis[ANALOG_INPUT_NUM];

} Analog_HandleTypeDef;

typedef struct _Pedals_ConfigHandleTypeDef {
  Analog_HandleTypeDef *hw_analog;
} Pedals_ConfigHandleTypeDef;

typedef struct _Pedals_HandleTypeDef {
  Wheel_Status (*INIT)(struct _Pedals_HandleTypeDef *analog,
                       Pedals_ConfigHandleTypeDef *config);
  Wheel_Status (*DeINIT)(struct _Pedals_HandleTypeDef *analog);
  Wheel_Status (*GetState)(struct _Pedals_HandleTypeDef *analog);

  Pedals_ConfigHandleTypeDef Config;
  // 0 to 255. Written in SysTick (GetState), read in main (wheel_get_input)
  volatile uint8_t clutch;
  volatile uint8_t brake;
  volatile uint8_t throtle;
} Pedals_HandleTypeDef;

extern Analog_HandleTypeDef hAnalog;
extern Pedals_HandleTypeDef hPedals;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_HW_ANALOG_INPUT_H_ */
