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
 * sw_buttons.h
 *
 *  Created on: Jul 26, 2025
 *      Author: raffi
 */

#ifndef CORE_DEFINITIONS_SW_BUTTONS_H_
#define CORE_DEFINITIONS_SW_BUTTONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "hw_digital_input.h"

/* DEBOUNCING */
#define SAMPLE_TIME_US 1000U // period of the poll timer, applied in main.c
#define MAX_SAMPLES 65U // majority vote over this many samples
#define HALF_SAMPLES (MAX_SAMPLES / 2)
#define BUTTONS_BUFFER_SIZE MAX_SAMPLES

/*
 * KNOB
 * A rotary encoder on buttons 23 and 24. One detent is a sequence of 5 states
 * (see right/left_sequence_backwards in sw_buttons.c). Once it is seen, the
 * matching button is reported pressed for KNOB_LOCK_TIME_MS and the knob is
 * ignored meanwhile.
 */
#define GET_BIT(var, mask) ((mask) & (var))
#define	KNOB_LOCK_TIME_MS 30U
#define R_KNOB_BIT_MASK (1U << (23-1))
#define	L_KNOB_BIT_MASK (1U << (24-1))
#define RL_KNOB_BIT_MASK (R_KNOB_BIT_MASK | L_KNOB_BIT_MASK)
#define ROTATION_SEQUENCE_SIZE 5U
#define KNOB_LOCK_FLAG (1U << (7)) // a detent is being reported
#define KNOB_DIRECTION_FLAG (1U << (6)) // its direction: 0 = left, 1 = right

typedef struct {
  TIM_HandleTypeDef *htim; // one sample per period, see SAMPLE_TIME_US
  DigitalInput_HandleTypeDef *hw_buttons;
} Buttons_ConfigHandleTypeDef;

typedef struct _Buttons_HandleTypeDef {
  Wheel_Status (*INIT)(struct _Buttons_HandleTypeDef *buttons,
                       Buttons_ConfigHandleTypeDef *config);
  Wheel_Status (*DeINIT)(struct _Buttons_HandleTypeDef *buttons);
  Wheel_Status (*Start_TIM_POLL)(struct _Buttons_HandleTypeDef *buttons);
  Wheel_Status (*Stop_TIM_POLL)(struct _Buttons_HandleTypeDef *buttons);
  // Takes one sample. Call from the period-elapsed callback of htim
  Wheel_Status (*TIM_POLL_CB)(struct _Buttons_HandleTypeDef *buttons);
  // Refreshes buttons_state
  Wheel_Status (*GetState)(struct _Buttons_HandleTypeDef *buttons);

  Buttons_ConfigHandleTypeDef Config;

  // One bit per button, debounced. Written in SysTick (GetState) in several
  // steps, so main reads it with the interrupts off (wheel_get_input)
  volatile uint32_t buttons_state;

  /* PRIVATE */
  // Written in the TIM3 interrupt (TIM_POLL_CB), read in SysTick (GetState).
  // Neither volatile nor protected: both have priority 6 and cannot interrupt
  // each other. Review this if one of the two priorities changes
  uint32_t sample_buffer[BUTTONS_BUFFER_SIZE];
  uint32_t knob_rotation_sequence_buffer[ROTATION_SEQUENCE_SIZE];
  uint32_t knob_lock_init_time_ms;
  uint8_t knob_head;
  uint8_t knob_flags;
  uint16_t sample_head;
} Buttons_HandleTypeDef;

extern Buttons_HandleTypeDef hButtons;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_SW_BUTTONS_H_ */
