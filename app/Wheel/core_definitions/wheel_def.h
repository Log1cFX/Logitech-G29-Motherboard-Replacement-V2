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

/*
 * Central header: every module, the settings of the wheel and its handle.
 *
 * MODULES
 * Each module is a handle: function pointers, then Config, then its state.
 * hw_* modules deal with the hardware, sw_* modules turn their output into the
 * values the wheel uses. Calls go through the function pointers, so that an
 * implementation can be swapped without touching its users.
 * Config is copied by INIT and cleared by DeINIT, never written directly.
 * The peripherals themselves are set up by the CubeMX code of main.c.
 *
 * CONTEXTS
 * There is no scheduler: the work runs in interrupts and in main. A lower
 * priority number interrupts every higher one, equal numbers never interrupt
 * each other, and everything interrupts main.
 *
 *  priority | context               | what runs there
 *  ---------+-----------------------+---------------------------------------------
 *   0 / 1   | USB_HP / USB_LP       | usb_irq_handler (TinyUSB records the event)
 *   3       | TIM4                  | magnetometer TransmitRecieve_DMA
 *   3       | DMA1 ch4 / ch5 (SPI2) | magnetometer TxRxDone_CB, then sensor Update
 *   6       | TIM3                  | buttons TIM_POLL_CB
 *   6       | SysTick               | wheel_get_all_component_states
 *   10      | DMA1 ch1 (ADC)        | nothing: the ADC fills hAnalog.axis by itself
 *   -       | main                  | usb_task (USB, and the ffb library on host
 *           |                       | reports), calibration, control loop
 *
 * The priorities are set in main.c, stm32f1xx_hal_msp.c, stm32f1xx_hal_conf.h
 * (TICK_INT_PRIORITY) and usb_processing.c. Update this table when they change.
 *
 * A field shared between contexts says so in its comment and is volatile.
 * volatile is not atomic: an update made in several steps (x++, two fields that
 * go together) can still be seen half done by a context that interrupts it.
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

#define MAX_ROTATION_DEG 900 // between the two mechanical ends
#define ENDSTOP_DEG_OFFSET 15 // margin kept before each end
#define CONSTRAINED_ROTATION_DEG (MAX_ROTATION_DEG - (ENDSTOP_DEG_OFFSET * 2))

/* CALIBRATION */
#define CALIBRATION_FORCE 135 // out of MOTOR_MAX_FORCE

#ifdef DEBUG
#define CALIBRATION_MAX_TRIES 3
#else
	#define CALIBRATION_MAX_TRIES 250
#endif

#define CALIBRATION_RETRY_DELAY_MS 2000
#define CALIBRATION_MOTOR_START_DELAY_MS 40 // lets the wheel start moving
#define CALIBRATION_SAMPLE_PERIOD_MS 10 // between two position samples of a sweep
// An end is reached when steering_pos moves less than this between two samples
#define CALIBRATION_STALL_THRESHOLD 150
// Smallest end-to-end range accepted, in steering_pos units. About 64069 was
// measured in testing
#define CALIBRATION_MIN_RANGE 63750
#define RECENTER_TOLERANCE 150 // centered when |virtual_axis| is under this

/* CONTROL LOOP */
#define CONTROL_LOOP_PERIOD_MS 1
#define CONTROL_LOOP_RATE_HZ 1000.0f // the same period, for the ffb library

/* FORCE FEEDBACK */
#define FFB_AXIS_COUNT 1
#define FFB_STEERING_AXIS 0 // index of the steering axis in the ffb library
#define FFB_FILTER_PROFILE 0 // 0 = default profile, 1 = custom

// Low-pass filters of the speed and the acceleration (Hz, q = Q * 100).
// A lower cutoff smooths the derivatives of the position more, for more lag.
// Library defaults: {70, 55} for the speed, {55, 30} for the acceleration
#define METRICS_SPEED_FREQ_HZ 40
#define METRICS_SPEED_Q 55
#define METRICS_ACCEL_FREQ_HZ 25
#define METRICS_ACCEL_Q 30

// Effects computed by the wheel itself, 0 to 255
#define LOCAL_ENDSTOP_STRENGTH 250
#define LOCAL_IDLE_SPRING_STRENGTH 255
#define LOCAL_DAMPER_INTENSITY 0

// Low-pass filters of the host effects (Hz, q = Q * 100), each followed by the
// library default
#define FFB_CONSTANT_FILTER_FREQ_HZ 50 // 500, which filters nothing at 1 kHz
#define FFB_CONSTANT_FILTER_Q 60 // 70
#define FFB_DAMPER_FILTER_FREQ_HZ 15 // 30
#define FFB_FRICTION_FILTER_FREQ_HZ 15 // 50
#define FFB_INERTIA_FILTER_FREQ_HZ 5 // 15

typedef struct {
  uint32_t wheel_error_count; // main only
  // Set once by init_wheel_handle() (main), then read by every context.
  // SysTick already runs before that: wheel_get_all_component_states() checks
  // them for NULL
  DigitalInput_HandleTypeDef *hDigitalInput;
  Buttons_HandleTypeDef *hButtons;
  Magnetometer_HandleTypeDef *hMagnetometer;
  Sensor_HandleTypeDef *hSensor;
  Analog_HandleTypeDef *hAnalog;
  Pedals_HandleTypeDef *hPedals;
  Shifter_HandleTypeDef *hShifter;
  MotorDriver_HandleTypeDef *hMotorDriver;
  Actuator_HandleTypeDef *hActuator;
} Wheel_HandleTypeDef;

extern Wheel_HandleTypeDef wheel;

// Reads every control. Call from SysTick_Handler
Wheel_Status wheel_get_all_component_states();
// Call wheel_startup() once, then wheel_task() as often as possible. Main only
void wheel_startup();
void wheel_task();

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_WHEEL_DEF_H_ */
