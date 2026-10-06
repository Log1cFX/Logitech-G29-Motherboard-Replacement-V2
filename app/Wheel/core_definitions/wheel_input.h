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
 * wheel_input.h
 *
 *  Created on: Oct 6, 2026
 *      Author: raffi
 */

#ifndef CORE_DEFINITIONS_WHEEL_INPUT_H_
#define CORE_DEFINITIONS_WHEEL_INPUT_H_

/*
 * The state of all the controls of the wheel at one moment.
 * This is what the wheel gives to whatever sends it to the computer (the USB
 * module today). It is a plain copy : whoever gets it doesn't need to know the
 * modules it comes from, and all the values in it go together.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
	uint32_t buttons; // one bit per button, same as buttons_state in sw_buttons.h
	int16_t steering; // steering axis, uses full range of int16, 0 is the center
	uint8_t throttle; // pedals, from 0 to 255
	uint8_t brake;
	uint8_t clutch;
	uint8_t gear; // current gear from 0 to 7 where 0 is no gear
} wheel_input_t;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_WHEEL_INPUT_H_ */
