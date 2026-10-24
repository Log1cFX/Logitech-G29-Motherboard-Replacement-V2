/*
 * steeringwheel.c
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 *
 *  The wheel itself: its startup, its control loop, and the HAL callbacks that
 *  drive the modules
 */

#include "wheel_def.h"
#include "usb_processing.h"
#include "watchdog.h"
#include "wheel_command.h"
#include "wheel_log.h"
#include "wheel_settings.h"
#include "ffb/ffb_c.h"
#include "ffb/ffb_metrics_c.h"
#include "ffb/ffb_axis_local_c.h"

#include <stdlib.h>

#define MICROS_PER_MS 1000

#if WHEEL_SKIP_CALIBRATION
#warning "WHEEL_SKIP_CALIBRATION is 1: build for the bench, not for a wheel"
#endif

// The limits of the settings hold for this wheel
static_assert(WHEEL_FILE_SETTINGS_MAX_CALIBRATION_FORCE == MOTOR_MAX_FORCE,
    "the calibration force is out of MOTOR_MAX_FORCE");
static_assert(2 * WHEEL_FILE_SETTINGS_MAX_ENDSTOP_MARGIN_DEG < MAX_ROTATION_DEG,
    "the endstop margins leave no travel");

Wheel_HandleTypeDef wheel; // who uses what: see wheel_def.h
// Main only: the control loop, and the USB module when the host sends something
static ffb_lib_t *hFFB;
static ffb_metrics_t *metrics; // control loop only
static ffb_axis_local_t *local_effects; // control loop only
static uint32_t last_control_time; // HAL_GetTick() of the last control loop step

static void start_modules();
static void start_usb();
static void calibrate();
static void init_control_loop();
static void start_control_loop();
static Wheel_Status wheel_axis_calibration();
static void wheel_recenter();
static void wheel_delay(uint32_t ms);
static void wheel_get_input(wheel_input_t *input);
static bool tick();
static void control_loop_step();
static void usb_custom_task(usb_state_t *state);

// The init_* and create_* functions of wheel_startup(): a part of this file
// kept in its own header
#include "wheel_init.h"

/* STARTUP */

void wheel_startup() {
  // First: the errors of the inits below already go to the log
  wheel_log_init();
  // Then the settings, which every init below points to
  wheel_settings_init();
  wheel_log_set_settings(&wheel_settings->system.content);

  init_analog();
  init_buttons();
  init_sensor();
  init_motor_driver();
  init_ffb_library();
  init_wheel_handle();

  start_modules();
  // Before the calibration, so that the host does not time out meanwhile
  start_usb();
  if (!WHEEL_SKIP_CALIBRATION) {
    calibrate();
  }
  init_control_loop();
  // The commands of the PC run from the first wheel_task(). apply_settings()
  // needs the ffb objects
  wheel_command_init(&wheel, apply_settings);
  start_control_loop();
}

// Starts the scan of the ADC and the polling timers of the buttons and of the
// magnetometer
static void start_modules() {
  Magnetometer_HandleTypeDef *magnetometer = wheel.hMagnetometer;
  Analog_HandleTypeDef *analog = wheel.hAnalog;
  Buttons_HandleTypeDef *buttons = wheel.hButtons;
  if (analog->Start_CONTINUOUS_SCAN_DMA(analog) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
  if (buttons->Start_TIM_POLL(buttons) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
  if (magnetometer->Start_TIM_POLL(magnetometer) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
}

// Starts USB and waits until the host reads the reports. The watchdog starts
// first, fed by every usb_task() from there on. DEBUG builds leave it alone
static void start_usb() {
#ifndef DEBUG
  watchdog_init();
#endif
  usb_init(hFFB, usb_custom_task);
  while (usb_state() != USB_READY) {
    usb_task();
  }
}

// The ffb objects of the control loop, with the settings applied. It cannot
// run without them
static void init_control_loop() {
  metrics = create_metrics_helper();
  local_effects = create_local_effects();
  if (metrics == NULL || local_effects == NULL) {
    WHEEL_FATAL_ERROR(WHEEL_ERR_FFB);
  }
  apply_settings();
}

// Last step of the startup: the control loop runs from the first wheel_task(),
// under the supervision of the watchdog
static void start_control_loop() {
#ifndef DEBUG
  // Not on a board without its sensor: see WHEEL_SKIP_CALIBRATION
  if (!WHEEL_SKIP_CALIBRATION) {
    watchdog_start_supervision();
  }
#endif
  last_control_time = HAL_GetTick();
}

/* CONTROL LOOP */

void wheel_task() {
  usb_task();
  wheel_command_task();
  if (tick()) {
    control_loop_step();
  }
}

// Reads the controls, computes the force and applies it to the motor
static void control_loop_step() {
  static int32_t local_force = 0;
  static int32_t host_force = 0;
  static int32_t total_force = 0;
  static int16_t end_force = 0;

  // One copy of the controls per step, also sent to the host
  wheel_input_t input;
  wheel_get_input(&input);
  usb_set_input(&input);

  // Position in degrees, then speed and acceleration
  float degrees = input.steering * (float)(MAX_ROTATION_DEG / 2) / (float) INT16_MAX;
  ffb_axis_state_t st = ffb_metrics_update(metrics, degrees);

  // Effects of the host plus the effects computed by the wheel
  ffb_set_axis_state_s(hFFB, FFB_STEERING_AXIS, &st);
  ffb_calculate(hFFB);
  host_force = ffb_get_axis_torque(hFFB, FFB_STEERING_AXIS);
  local_force = ffb_axis_local_compute(local_effects, &st, ffb_is_active(hFFB));
  total_force = host_force + local_force;

  // From the int16 range to the range of the motor, at the strength of the
  // settings. total_force can exceed the int16 range (two forces added): the
  // clamp catches it. At most 2 * INT16_MAX * 255 * 100: fits in 32 bits
  int32_t strength_pct = wheel_settings->ffb.content.strength_pct;
  int32_t force = total_force * MOTOR_MAX_FORCE * strength_pct / (INT16_MAX * 100);
  if (force > MOTOR_MAX_FORCE) {
    force = MOTOR_MAX_FORCE;
  } else if (force < MOTOR_MIN_FORCE) {
    force = MOTOR_MIN_FORCE;
  }
  end_force = (int16_t)force;

  // A refused force is logged by the actuator
  wheel.hActuator->Apply_Force(wheel.hActuator, (int16_t)end_force);

  watchdog_checkin(WATCHDOG_SOURCE_CONTROL_LOOP);
}

/* CALIBRATION */

// Calibrates the axis, then recenters the wheel. Retried until it succeeds,
// each failure is logged from the CALIBRATION_MAX_TRIES-th on
static void calibrate() {
  uint8_t calibration_tries = 0;
  while (wheel_axis_calibration() == WHEEL_ERROR) {
    wheel_delay(CALIBRATION_RETRY_DELAY_MS);
    // Stops counting there
    if (calibration_tries < CALIBRATION_MAX_TRIES) {
      calibration_tries++;
    }
    if (calibration_tries >= CALIBRATION_MAX_TRIES) {
      WHEEL_LOG_ERROR(WHEEL_ERR_CALIBRATION);
    }
  }
  wheel_recenter();
}

// One sweep: pushes with force until the wheel stalls, keeping the farthest
// position in *extremum.
// dir = +1 goes right and tracks a maximum, dir = -1 goes left and tracks a
// minimum
static void wheel_calib_sweep(Sensor_HandleTypeDef *sensor, int16_t force,
                              volatile int32_t *extremum, int dir) {
  uint16_t stall_threshold = wheel_settings->calibration.content.stall_threshold;
  int32_t previous = sensor->steering_pos;
  wheel.hActuator->Apply_Force(wheel.hActuator, force);
  wheel_delay(CALIBRATION_MOTOR_START_DELAY_MS);
  int16_t acceleration = sensor->steering_pos - previous;
  while (abs(acceleration) > stall_threshold) {
    if (dir * sensor->steering_pos > dir * *extremum) {
      *extremum = sensor->steering_pos;
    }
    previous = sensor->steering_pos;
    wheel_delay(CALIBRATION_SAMPLE_PERIOD_MS);
    acceleration = sensor->steering_pos - previous;
  }
}

// Both sweeps with the force of the settings, then the scale of the axis. The
// range found must reach min_range
static Wheel_Status wheel_axis_calibration() {
  Sensor_HandleTypeDef *sensor = wheel.hSensor;
  const wheel_file_settings_calibration_t *calibration =
      &wheel_settings->calibration.content;
  int16_t force = (int16_t)calibration->force;

  wheel_calib_sweep(sensor, -force, &sensor->min, -1);
  wheel_calib_sweep(sensor, force, &sensor->max, +1);
  wheel.hActuator->Apply_Force(wheel.hActuator, 0);

  sensor->axis_scale = (float)(INT16_MAX) / (sensor->distance / 2);
  return (sensor->distance < calibration->min_range) ? WHEEL_ERROR : WHEEL_OK;
}

// Pushes towards the center until the axis is within recenter_tolerance
static void wheel_recenter() {
  const wheel_file_settings_calibration_t *calibration =
      &wheel_settings->calibration.content;
  int16_t force = (int16_t)calibration->force;
  int8_t sign = (wheel.hSensor->virtual_axis > 0) ? 1 : -1;
  while (abs(wheel.hSensor->virtual_axis) > calibration->recenter_tolerance) {
    wheel.hActuator->Apply_Force(wheel.hActuator, force * -sign);
    usb_task();
  }
  wheel.hActuator->Apply_Force(wheel.hActuator, 0);
}

/* HELPERS */

// HAL_Delay() that keeps answering the host. For main, once USB is started
static void wheel_delay(uint32_t ms) {
  uint32_t start = HAL_GetTick();
  uint32_t wait = ms + 1; // one more tick, like HAL_Delay(): waits at least ms
  while ((HAL_GetTick() - start) < wait) {
    usb_task();
  }
}

// True once every CONTROL_LOOP_PERIOD_MS
static bool tick() {
  uint32_t current_time = HAL_GetTick();
  if (current_time - last_control_time < CONTROL_LOOP_PERIOD_MS) {
    return false;
  }
  last_control_time = current_time;
  return true;
}

// Called by every usb_task(): the only place that feeds the watchdog (Release),
// and the one that sends the logs
static void usb_custom_task(usb_state_t *state) {
  UNUSED(state);
#ifndef DEBUG
  watchdog_task();
#endif
  wheel_log_task();
}

/* CONTROLS */

// Runs in SysTick (priority 6), every millisecond
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

// Copies the controls for main. SysTick writes them and can interrupt main
// anywhere: the interrupts are off during the copy, so that every value comes
// from the same update
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

/* HAL CALLBACKS */
// Unused: the ADC fills hAnalog.axis by itself (continuous scan with DMA)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  UNUSED(hadc);
}

// End of a magnetometer transfer. Runs in the SPI2 DMA interrupt (priority 3)
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (wheel.hMagnetometer->Config.hspi->Instance == hspi->Instance) {
    // WHEEL_OK only when the frame passed every check, CRC included
    if (wheel.hMagnetometer->TxRxDone_CB(wheel.hMagnetometer) == WHEEL_OK) {
      watchdog_checkin(WATCHDOG_SOURCE_SENSOR);
    }
    wheel.hSensor->Update(wheel.hSensor);
  }
}

// TIM4 interrupt (priority 3): starts a magnetometer transfer.
// TIM3 interrupt (priority 6): takes one sample of the buttons
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  Magnetometer_HandleTypeDef *hw_magnetometer = wheel.hMagnetometer;
  if (hw_magnetometer->Config.htim->Instance == htim->Instance) {
    hw_magnetometer->TransmitRecieve_DMA(hw_magnetometer);
  }
  Buttons_HandleTypeDef *hButtons = wheel.hButtons;
  if (hButtons->Config.htim->Instance == htim->Instance) {
    hButtons->TIM_POLL_CB(hButtons);
  }
}

// The magnetometer transfer failed: TxRxDone_CB() will not run for it. Runs in
// the SPI2 DMA interrupt (priority 3)
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  Magnetometer_ConfigHandleTypeDef *config = &wheel.hMagnetometer->Config;
  if (config->hspi->Instance == hspi->Instance) {
    // Ends the frame, as TxRxDone_CB() would: the next one starts clean
    HAL_GPIO_WritePin(config->SS_port, config->SS_pin, 1);
    WHEEL_LOG_ERROR(WHEEL_ERR_SENSOR_SPI);
  }
}

// The DMA of the ADC failed: the pedals and the shifter may keep old values.
// Runs in the DMA1 channel 1 interrupt (priority 10)
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
  UNUSED(hadc);
  WHEEL_LOG_ERROR(WHEEL_ERR_ANALOG);
}
