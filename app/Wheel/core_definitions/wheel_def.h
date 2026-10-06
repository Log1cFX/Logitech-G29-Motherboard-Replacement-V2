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

/*
 * WHO RUNS WHERE
 *
 * There is no scheduler. The tasks run in interrupts and the priorities decide
 * who can interrupt who : a lower number can interrupt every bigger number,
 * equal numbers never interrupt each other, everything interrupts the main thread.
 *
 *  priority | context                      | what runs there
 *  ---------+------------------------------+------------------------------------------------
 *   0 / 1   | USB_HP / USB_LP              | usb_irq_handler (TinyUSB only notes the event for usb_task)
 *   3       | TIM4                         | magnetometer TransmitRecieve_DMA
 *   3       | DMA1 ch4 / ch5 (SPI2)        | magnetometer TxRxDone_CB, then sensor Update
 *   4 / 5   | EXTI1 / EXTI0                | nothing, the two lines are turned off at startup
 *   6       | TIM3                         | buttons TIM_POLL_CB
 *   6       | SysTick                      | wheel_get_all_component_states (every GetState / GetAxis)
 *   10      | DMA1 ch1 (ADC)               | nothing, the ADC fills hAnalog.axis through DMA by itself
 *   -       | main thread                  | usb_task (all of USB, and the ffb library when the host
 *           |                              | sends something), calibration, force calculation, Apply_Force
 *
 * The priorities are set in main.c, stm32f1xx_hal_msp.c, usb_processing.c (USB)
 * and stm32f1xx_hal_conf.h (TICK_INT_PRIORITY). Update this table if they change.
 *
 * Every field that is written in one context and read in another one has a
 * comment saying so next to it, and is volatile so the compiler always reads
 * the real value from memory instead of a copy it kept in a register.
 * volatile doesn't make anything atomic : something that takes more than one
 * step to update (x++, or two fields that go together) can still be seen
 * half updated by a context that interrupts the writer.
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

#define MAX_ROTATION_DEG 900
#define ENDSTOP_DEG_OFFSET 15
#define CONSTRAINED_ROTATION_DEG (MAX_ROTATION_DEG - (ENDSTOP_DEG_OFFSET * 2))

/* CALIBRATION */
#define CALIBRATION_FORCE 135

#ifdef DEBUG
	#define CALIBRATION_MAX_TRIES 3
#else
	#define CALIBRATION_MAX_TRIES 250
#endif

// pause between two calibration attempts
#define CALIBRATION_RETRY_DELAY_MS 2000
// time given to the motor to get the wheel moving at the start of a sweep
#define CALIBRATION_MOTOR_START_DELAY_MS 40
// time between two position samples during a sweep
#define CALIBRATION_SAMPLE_PERIOD_MS 10
// the wheel is considered stopped (it reached the end) when its position
// changes by less than this between two samples (steering_pos units)
#define CALIBRATION_STALL_THRESHOLD 150
// smallest end to end range accepted as a valid calibration (steering_pos units)
#define CALIBRATION_MIN_RANGE 63750
// the wheel is considered centered when |virtual_axis| is under this
#define RECENTER_TOLERANCE 150

/* CONTROL LOOP */
// period of the force calculation in the main loop
#define CONTROL_LOOP_PERIOD_MS 1
// same thing in Hz for the ffb library, has to match CONTROL_LOOP_PERIOD_MS
#define CONTROL_LOOP_RATE_HZ 1000.0f

/* FORCE FEEDBACK */
#define FFB_AXIS_COUNT 1
#define FFB_STEERING_AXIS 0 // index of the steering axis in the ffb library
#define FFB_FILTER_PROFILE 0 // 0 = default profile, 1 = custom

// low-pass filters applied to speed and acceleration (freq in Hz, q is Q*100)
// library defaults are {70,55} for speed and {55,30} for acceleration
#define METRICS_SPEED_FREQ_HZ 40
#define METRICS_SPEED_Q 55
#define METRICS_ACCEL_FREQ_HZ 25
#define METRICS_ACCEL_Q 30

// effects computed by the wheel itself (0..255)
#define LOCAL_ENDSTOP_STRENGTH 250
#define LOCAL_IDLE_SPRING_STRENGTH 255
#define LOCAL_DAMPER_INTENSITY 0

// filters applied to the effects sent by the host (freq in Hz, q is Q*100)
#define FFB_CONSTANT_FILTER_FREQ_HZ 50 // was 500 (no-op at 1 kHz)
#define FFB_CONSTANT_FILTER_Q 60 // was 70
#define FFB_DAMPER_FILTER_FREQ_HZ 15 // was 30
#define FFB_FRICTION_FILTER_FREQ_HZ 15 // was 50
#define FFB_INERTIA_FILTER_FREQ_HZ 5 // was 15

typedef struct {
	uint32_t wheel_error_count; // main thread only
	// The pointers are written once by init_wheel_handle() (main thread) and
	// never change after that. They are read by every context.
	// SysTick is already running before they are set, that is why
	// wheel_get_all_component_states() checks them for NULL.
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

// defined in steeringwheel.c
extern Wheel_HandleTypeDef wheel;

Wheel_Status wheel_get_all_component_states();
void wheel_startup();

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_WHEEL_DEF_H_ */
