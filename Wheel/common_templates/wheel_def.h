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
 * wheel_def.h
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#ifndef COMMON_TEMPLATES_WHEEL_DEF_H_
#define COMMON_TEMPLATES_WHEEL_DEF_H_

/* This file contains all the imports for every template and the wheel handle.
 * I should also probably tell you what I mean by "template".
 * I wanted to abstract the functioning of every module as much as possible to make changing the code easier.
 * Let's say the buttons. It is a module. Each module has a hardware part and a software part.
 * The hardware part deals with hardware specific stuff, which doesn't mean it doesn't use low level functions.
 * The software part uses lower stack's output, through standardized functions, to do calculations on a higher level.
 * The next step is my code, which seamlessly uses the template functions to get data.
 * I decided to make it work that way to be able to swap the modules easily,
 * that is also why dynamically called function pointers have been chosen over compile time calls.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "hw_digital_input.h"
#include "sw_buttons.h"
#include "hw_magnetometer.h"
#include "sw_sensor.h"
#include "hw_analog_input.h"
#include "sw_shifter.h"
#include "hw_motor_driver.h"
#include "sw_actuator.h"
#include "app_usb_hid.h"

struct _USB_HID_HandleTypeDef;

typedef struct {
	uint16_t usb_send_report_swit_pin;
	uint16_t usb_process_data_pin;
	uint16_t swit_pin_2;
}Wheel_SWIT_HandleTypeDef;

typedef struct {
	uint32_t wheel_error_count;
	Buttons_HandleTypeDef *hButtons;
	Sensor_HandleTypeDef *hSensor;
	Pedals_HandleTypeDef *hPedals;
	Shifter_HandleTypeDef *hShifter;
	Actuator_HandleTypeDef *hActuator;
	Wheel_SWIT_HandleTypeDef hSwit;
	struct _USB_HID_HandleTypeDef *hUsbHid;
}Wheel_HandleTypeDef;

Wheel_Status wheel_get_all_component_states();

#ifdef __cplusplus
}
#endif

#endif /* COMMON_TEMPLATES_WHEEL_DEF_H_ */
