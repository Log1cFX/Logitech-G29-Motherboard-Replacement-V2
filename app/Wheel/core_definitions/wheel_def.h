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
 * A header of hw_* is an interface. A part that implements it is named after
 * itself: its file, its functions and its handle, declared in its own header
 * (hw_mlx90363.c, MLX90363_INIT, hmlx90363 in hw_mlx90363.h). Only
 * wheel_init.h names the parts. hw_analog_input, the ADC of the MCU, is no
 * part of its own: its handles stay in its header.
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
 *   -2 / -1 | NMI / HardFault       | the fatal path (wheel_fatal.c)
 *   0       | MemManage, BusFault,  | the fatal path (wheel_fatal.c)
 *           | UsageFault            |
 *   0 / 1   | USB_HP / USB_LP       | usb_irq_handler (TinyUSB records the event)
 *   3       | TIM4                  | magnetometer TransmitRecieve_DMA
 *   3       | DMA1 ch4 / ch5 (SPI2) | magnetometer TxRxDone_CB, then sensor Update
 *   6       | TIM3                  | buttons TIM_POLL_CB
 *   6       | SysTick               | wheel_get_all_component_states
 *   10      | DMA1 ch1 (ADC)        | nothing: the ADC fills hAnalog.axis by itself
 *   -       | main                  | usb_task (USB, the ffb library on host
 *           |                       | reports, the file link of usb_msg.h, the
 *           |                       | logs of wheel_log.h), calibration, the
 *           |                       | commands of the PC (wheel_command.h),
 *           |                       | control loop
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

// The values that can be tuned are settings, not defines: see wheel_settings.h

#define MAX_ROTATION_DEG 900 // between the two mechanical ends

/* CALIBRATION */
// 1: for a board on the bench, without its motor and its sensor. The startup
// skips the calibration and the recentering, which never succeed there, and
// the watchdog stays in its startup phase: no sensor frame would ever check in
// (see watchdog.h). The steering axis stays at 0.
// 0 on a wheel: without the calibration it does not know where its ends are
#define WHEEL_SKIP_CALIBRATION 1

// What the firmware logs: see WHEEL_LOG_LEVEL in wheel_log.h

#ifdef DEBUG
#define CALIBRATION_MAX_TRIES 3
#else
#define CALIBRATION_MAX_TRIES 250
#endif

#define CALIBRATION_RETRY_DELAY_MS 2000
#define CALIBRATION_MOTOR_START_DELAY_MS 40 // lets the wheel start moving
#define CALIBRATION_SAMPLE_PERIOD_MS 10 // between two position samples of a sweep

/* CONTROL LOOP */
#define CONTROL_LOOP_PERIOD_MS 1
#define CONTROL_LOOP_RATE_HZ 1000.0f // the same period, for the ffb library

/* FORCE FEEDBACK */
#define FFB_AXIS_COUNT 1
#define FFB_STEERING_AXIS 0 // index of the steering axis in the ffb library
// Filter profile of the library edited with the filters of the settings
#define FFB_FILTER_PROFILE 0 // 0 = default profile, 1 = custom

typedef struct {
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
}Wheel_HandleTypeDef;

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
