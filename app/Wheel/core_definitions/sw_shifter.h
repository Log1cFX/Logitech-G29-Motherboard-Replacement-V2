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

#ifndef CORE_DEFINITIONS_SW_SHIFTER_H_
#define CORE_DEFINITIONS_SW_SHIFTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "hw_analog_input.h"
#include "wheel_file_settings.h"

// The travel is split into a grid, each cell is a gear or neutral
#define SHIFTER_GRID_COLS 3
#define SHIFTER_GRID_ROWS 4
// Column shift applied while the modifier is pressed, to reach the extra
// columns of the gear table
#define SHIFTER_MODIFIER_COL_OFFSET 2

typedef struct {
  uint16_t x;
  uint16_t y;
}Point;

typedef struct {
  Analog_HandleTypeDef *hw_analog;
  uint16_t modifier_pin;
  GPIO_TypeDef *modifier_port;
  // The corners of the travel, in the settings image (wheel_settings.h). Read
  // at every GetState: a change applies at once
  const wheel_file_settings_shifter_t *settings;
}Shifter_ConfigHandleTypeDef;

typedef struct _Shifter_HandleTypeDef {
  Wheel_Status (*INIT)(struct _Shifter_HandleTypeDef *shifter,
      Shifter_ConfigHandleTypeDef *config);
  Wheel_Status (*DeINIT)(struct _Shifter_HandleTypeDef *shifter);
  Wheel_Status (*GetState)(struct _Shifter_HandleTypeDef *shifter); // refreshes gear

  Shifter_ConfigHandleTypeDef Config;
  // 0 (no gear) to 7. Written in SysTick (GetState), read in main (wheel_get_input)
  volatile uint8_t gear;

  // The state of the implementation, in its .c. Set with the handle, never
  // changed
  void *priv;
}Shifter_HandleTypeDef;

extern Shifter_HandleTypeDef hShifter;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_SW_SHIFTER_H_ */
