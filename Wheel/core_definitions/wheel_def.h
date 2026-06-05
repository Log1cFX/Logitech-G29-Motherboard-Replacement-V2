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

#ifndef CORE_DEFINITIONS_WHEEL_DEF_H_
#define CORE_DEFINITIONS_WHEEL_DEF_H_

/* This file contains all the imports for every "template" and the wheel handle.
 * I should also probably tell you what I mean by "template" (note: this file was initially named common_templates).
 * I wanted to abstract the functioning of every module as much as possible to make changing the implementation easier.
 * Let's say the buttons. It is a module. Each module has a hardware part and a software part.
 * The hardware part deals with hardware specific stuff.
 * The software part uses lower hardware's output, through standardized functions, to do calculations on a higher level,
 * which doesn't mean it doesn't use low level functions like writing on pin.
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

#define STEERING_RESISTANCE_START 31000
#define CALIBRATION_FORCE 150

#ifdef RELEASE
#define CALIBRATION_MAX_TRIES 250
#endif
#ifdef DEBUG
#define CALIBRATION_MAX_TRIES 3
#endif


typedef struct {
	uint32_t wheel_error_count;
	DigitalInput_HandleTypeDef *hDigitalInput;
	Buttons_HandleTypeDef *hButtons;
	Magnetometer_HandleTypeDef *hMagnetometer;
	Sensor_HandleTypeDef *hSensor;
	Analog_HandleTypeDef *hAnalog;
	Pedals_HandleTypeDef *hPedals;
	Shifter_HandleTypeDef *hShifter;
	MotorDriver_HandleTypeDef *hMotorDriver;
	Actuator_HandleTypeDef *hActuator;
}Wheel_HandleTypeDef;

Wheel_Status wheel_get_all_component_states();
void wheel_startup();

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_WHEEL_DEF_H_ */
