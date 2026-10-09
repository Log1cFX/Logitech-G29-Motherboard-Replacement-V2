/*
 * startupWheel.c
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#include "wheel_def.h"
#include "usb_processing.h"
#include "watchdog.h"
#include "ffb/ffb_c.h"
#include "ffb/ffb_metrics_c.h"
#include "ffb/ffb_axis_local_c.h"

#include <stdlib.h>

#define MICROS_PER_MS 1000

// see the comments in wheel_def.h for who uses what
Wheel_HandleTypeDef wheel;
// Created once by init_ffb_library(). Only used by the main thread : by the
// control loop below, and by the USB module (it gets it in usb_init) when
// the host sends something
static ffb_lib_t *hFFB;
// Created once at the end of wheel_startup(). Only used by the control loop
static ffb_metrics_t *metrics;
static ffb_axis_local_t *local_effects;
// last time the control loop ran (HAL_GetTick), main thread only
static uint32_t last_control_time;

static void register_initialization_error();

static Wheel_Status wheel_axis_calibration();
static void wheel_recenter();
static void wheel_delay(uint32_t ms);
static void wheel_get_input(wheel_input_t *input);
static bool tick();
static void control_loop_step();
#ifndef DEBUG
static void usb_custom_task(usb_state_t *state);
#endif

// The init_* and create_* functions used by wheel_startup(). They are a part
// of this file that lives in wheel_init.h, which is included only here
#include "wheel_init.h"

void wheel_startup() {

  /* INIT */
  init_analog();
  init_buttons();
  init_sensor();
  init_motor_driver();
  init_ffb_library();
  init_wheel_handle();

  /* START MODULES */
  Magnetometer_HandleTypeDef *magnetometer = wheel.hMagnetometer;
  Analog_HandleTypeDef *analog = wheel.hAnalog;
  Buttons_HandleTypeDef *buttons = wheel.hButtons;
  if (analog->Start_CONTINUOUS_SCAN_DMA(analog) == WHEEL_ERROR) {
    register_initialization_error();
  }
  if (buttons->Start_TIM_POLL(buttons) == WHEEL_ERROR) {
    register_initialization_error();
  }
  if (magnetometer->Start_TIM_POLL(magnetometer) == WHEEL_ERROR) {
    register_initialization_error();
  }

  // driver needs to be initialized before we go into calibration
  // we don't want a timeout to occur on usb port
#ifdef DEBUG
  usb_init(hFFB, NULL);
#else
	watchdog_init();
	usb_init(hFFB, usb_custom_task);
#endif
  while (usb_state() != USB_READY) {
    usb_task();
  }

  // try calibration until succeeds or the max attempts number is reached
  uint8_t calibration_tries = 0;
  while (wheel_axis_calibration() == WHEEL_ERROR) {
    wheel_delay(CALIBRATION_RETRY_DELAY_MS);
    calibration_tries++;
    if (calibration_tries >= CALIBRATION_MAX_TRIES) {
      register_initialization_error();
    }
  }
  wheel_recenter();

  metrics = create_metrics_helper();
  local_effects = create_local_effects();
  init_filter_preset();

#ifndef DEBUG
	watchdog_start_supervision();
#endif
  last_control_time = HAL_GetTick();
}

// One pass of the main loop. To call from the main thread, as often as
// possible, once wheel_startup() has returned
void wheel_task() {
  usb_task();
  if (tick()) {
    control_loop_step();
  }
}

// reads the controls, computes the force and applies it on the motor
static void control_loop_step() {
  // force variables
  static int32_t local_force = 0;
  static int32_t host_force = 0;
  static int32_t total_force = 0;
  static int16_t end_force = 0;

  // one copy of the controls for this pass,
  // the usb module sends it to the host
  wheel_input_t input;
  wheel_get_input(&input);
  usb_set_input(&input);

  // compute current degrees and update the axis state
  float degrees = input.steering * (float)(MAX_ROTATION_DEG / 2) / (float) INT16_MAX;
  ffb_axis_state_t st = ffb_metrics_update(metrics, degrees);

  // compute forces
  ffb_set_axis_state_s(hFFB, FFB_STEERING_AXIS, &st);
  ffb_calculate(hFFB);
  host_force = ffb_get_axis_torque(hFFB, FFB_STEERING_AXIS);
  local_force = ffb_axis_local_compute(local_effects, &st, ffb_is_active(hFFB));
  total_force = host_force + local_force;

  // remap and clamp the final force
  end_force = remapf(INT16_MIN, INT16_MAX, total_force,
  MOTOR_MIN_FORCE,
                     MOTOR_MAX_FORCE);
  end_force = clamp(end_force, MOTOR_MIN_FORCE, MOTOR_MAX_FORCE);

  // apply the force on the motor
  if (wheel.hActuator->Apply_Force(wheel.hActuator, (int16_t)end_force) == WHEEL_ERROR) {
    wheel.wheel_error_count++;
  }

  watchdog_checkin(WATCHDOG_SOURCE_CONTROL_LOOP);
}

/* One calibration sweep: push with `force` until the wheel stalls
 * (acceleration drops below threshold), tracking the extremum.
 * dir = +1 goes right, tracking a maximum (pos > extremum).
 * dir = -1 goes left,  tracking a minimum (pos < extremum). */
static void wheel_calib_sweep(Sensor_HandleTypeDef *sensor, int16_t force,
                              volatile int32_t *extremum, int dir) {
  int32_t previous = sensor->steering_pos;
  wheel.hActuator->Apply_Force(wheel.hActuator, force);
  wheel_delay(CALIBRATION_MOTOR_START_DELAY_MS); // A: let the motor start
  int16_t acceleration = sensor->steering_pos - previous;
  while (abs(acceleration) > CALIBRATION_STALL_THRESHOLD) {
    // B: finding out if current position is the farthest
    if (dir * sensor->steering_pos > dir * *extremum) {
      *extremum = sensor->steering_pos;
    }
    // C: calculating acceleration
    previous = sensor->steering_pos;
    wheel_delay(CALIBRATION_SAMPLE_PERIOD_MS);
    acceleration = sensor->steering_pos - previous;
  }
}

static Wheel_Status wheel_axis_calibration() {
  Sensor_HandleTypeDef *sensor = wheel.hSensor;

  wheel_calib_sweep(sensor, -CALIBRATION_FORCE, &sensor->min, -1); // left
  wheel_calib_sweep(sensor, CALIBRATION_FORCE, &sensor->max, +1); // right
  wheel.hActuator->Apply_Force(wheel.hActuator, 0);

  sensor->axis_scale = (float)(INT16_MAX) / (sensor->distance / 2);
  // In testing, the range is ~64069
  return (sensor->distance < CALIBRATION_MIN_RANGE) ? WHEEL_ERROR : WHEEL_OK;
}

static void wheel_recenter() {
  int8_t sign = (wheel.hSensor->virtual_axis > 0) ? 1 : -1;
  while (abs(wheel.hSensor->virtual_axis) > RECENTER_TOLERANCE) {
    wheel.hActuator->Apply_Force(wheel.hActuator,
    CALIBRATION_FORCE * -sign);
    usb_task();
  }
  wheel.hActuator->Apply_Force(wheel.hActuator, 0);
}

// HAL_Delay for the main thread once usb is started : it waits the same
// time, but it keeps answering the host while it waits
static void wheel_delay(uint32_t ms) {
  uint32_t start = HAL_GetTick();
  // one tick more than asked, like HAL_Delay, to wait at least ms
  uint32_t wait = ms + 1;
  while ((HAL_GetTick() - start) < wait) {
    usb_task();
  }
}

// true once every CONTROL_LOOP_PERIOD_MS : it is time to run the control loop
static bool tick() {
  uint32_t current_time = HAL_GetTick();
  if (current_time - last_control_time < CONTROL_LOOP_PERIOD_MS) {
    return false;
  }
  last_control_time = current_time;
  return true;
}

#ifndef DEBUG
// Given to the USB module, which calls it in every usb_task().
// It is the only place where the watchdog is fed
static void usb_custom_task(usb_state_t *state) {
	UNUSED(state);
	watchdog_task();
}
#endif

static void register_initialization_error() {
#ifdef DEBUG
  Error_Handler();
#else
	wheel.wheel_error_count++;
#endif
}

/* 		APPLICATION SPECIFIC FUNCTIONS 		*/

// context: SysTick (priority 6), called from SysTick_Handler every millisecond
Wheel_Status wheel_get_all_component_states() {
  watchdog_checkin(WATCHDOG_SOURCE_SYSTICK);
  if ((wheel.hPedals == NULL) || (wheel.hShifter == NULL)) {
    return WHEEL_ERROR;
  }
  if (wheel.hButtons == NULL) {
    return WHEEL_ERROR;
  }
  if (wheel.hSensor == NULL) {
    return WHEEL_ERROR;
  }
  Wheel_Status ret = WHEEL_OK;
  ret |= wheel.hButtons->GetState(wheel.hButtons);
  ret |= wheel.hPedals->GetState(wheel.hPedals);
  ret |= wheel.hSensor->GetAxis(wheel.hSensor);
  ret |= wheel.hShifter->GetState(wheel.hShifter);
  return ret;
}

// Copies the state of the controls for the main thread.
// SysTick writes these fields (wheel_get_all_component_states) and it can
// interrupt the main thread anywhere, so the interrupts are turned off for
// the few instructions of the copy : all the values come from the same update
static void wheel_get_input(wheel_input_t *input) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  input->buttons = wheel.hButtons->buttons_state;
  input->steering = wheel.hSensor->virtual_axis;
  input->throttle = wheel.hPedals->throtle;
  input->brake = wheel.hPedals->brake;
  input->clutch = wheel.hPedals->clutch;
  input->gear = wheel.hShifter->gear;
  __set_PRIMASK(primask);
}

/*		HARDWARE CALLBACK FUNCTIONS		 	*/
// ADC callbacks not used because ADC fills the values in continuous scan mode,
// paired up with DMA, meaning that we never have to worry about it.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  UNUSED(hadc);
}

// Called at the end of a transfer to process the raw data received by the sensor
// context: SPI2 DMA interrupt (priority 3)
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (wheel.hMagnetometer->Config.hspi->Instance == hspi->Instance) {
    // WHEEL_OK only when the frame passed every check (CRC included)
    if (wheel.hMagnetometer->TxRxDone_CB(wheel.hMagnetometer) == WHEEL_OK) {
      watchdog_checkin(WATCHDOG_SOURCE_SENSOR);
    }
    wheel.hSensor->Update(wheel.hSensor);
  }
}

// 1. Used to start, periodically, the transmission with the magnetometer (steering)
// 2. Used to periodically read the buttons' state (for debouncing)
// context: TIM4 interrupt (priority 3) for 1, TIM3 interrupt (priority 6) for 2
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  // 1
  Magnetometer_HandleTypeDef *hw_magnetometer = wheel.hMagnetometer;
  if (hw_magnetometer->Config.htim->Instance == htim->Instance) {
    hw_magnetometer->TransmitRecieve_DMA(hw_magnetometer);
  }
  // 2
  Buttons_HandleTypeDef *hButtons = wheel.hButtons;
  if (hButtons->Config.htim->Instance == htim->Instance) {
    hButtons->TIM_POLL_CB(hButtons);
  }
}
