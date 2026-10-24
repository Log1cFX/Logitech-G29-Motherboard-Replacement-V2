/*
 * wheel_init.h
 *
 *  Created on: 8 oct. 2026
 *      Author: raffi
 *
 *  Initialization of the modules of the wheel and of the ffb library.
 *
 *  Not a regular header: it is a part of steeringwheel.c, included only there.
 *  It uses wheel and hFFB, defined above the include. A module that refuses
 *  its config is logged as WHEEL_ERR_INIT, at the line of its call
 */

#ifndef WHEEL_INIT_H_
#define WHEEL_INIT_H_

// The parts of the wheel: the only place that names them (see MODULES in
// wheel_def.h)
#include "hw_g29_buttons.h"
#include "hw_mlx90363.h"
#include "hw_bts7960.h"

// Effect types of the HID PID usage table whose filters the settings set. The
// library has them in ffb_defs.h, which is C++ only
#define FFB_TYPE_CONSTANT 0x01u
#define FFB_TYPE_DAMPER 0x09u
#define FFB_TYPE_INERTIA 0x0Au
#define FFB_TYPE_FRICTION 0x0Bu

static void init_wheel_handle();
static void init_buttons();
static void init_sensor();
static void init_analog();
static void init_motor_driver();
static void init_ffb_library();
static float endstop_rotation_deg();
static ffb_axis_local_t* create_local_effects();
static ffb_metrics_t* create_metrics_helper();
static void apply_settings();

static void init_wheel_handle() {
  wheel.hDigitalInput = &hG29Buttons;
  wheel.hButtons = &hButtons;
  wheel.hMagnetometer = &hmlx90363;
  wheel.hSensor = &hSensor;
  wheel.hAnalog = &hAnalog;
  wheel.hPedals = &hPedals;
  wheel.hShifter = &hShifter;
  wheel.hMotorDriver = &hbts7960;
  wheel.hActuator = &hActuator;
}

static void init_buttons() {
  DigitalInput_ConfigHandleTypeDef config1 = {0};
  config1.buttons_port = GPIOC;
  config1.clk_pin = BUTTON_CLK_Pin;
  config1.lock_pin = BUTTON_LOCK_Pin;
  config1.in_pin = BUTTON_IN_Pin;
  if (hG29Buttons.INIT(&hG29Buttons, &config1) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }

  Buttons_ConfigHandleTypeDef config2 = {0};
  config2.htim = &htim3;
  config2.hw_buttons = &hG29Buttons;
  config2.settings = &wheel_settings->buttons.content;
  if (hButtons.INIT(&hButtons, &config2) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
}

static void init_sensor() {
  Magnetometer_ConfigHandleTypeDef config1 = {0};
  config1.hspi = &hspi2;
  config1.htim = &htim4;
  config1.SS_port = SPI2_SS_GPIO_Port;
  config1.SS_pin = SPI2_SS_Pin;
  if (hmlx90363.INIT(&hmlx90363, &config1) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }

  Sensor_ConfigHandleTypeDef config2 = {0};
  config2.hw_magnetometer = &hmlx90363;
  if (hSensor.INIT(&hSensor, &config2) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
}

static void init_analog() {
  Analog_ConfigHandleTypeDef config1 = {0};
  config1.hadc = &hadc1;
  if (hAnalog.INIT(&hAnalog, &config1) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }

  Pedals_ConfigHandleTypeDef config2 = {0};
  config2.hw_analog = &hAnalog;
  config2.settings = &wheel_settings->pedals.content;
  if (hPedals.INIT(&hPedals, &config2) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }

  Shifter_ConfigHandleTypeDef config3 = {0};
  config3.hw_analog = &hAnalog;
  config3.modifier_port = SHIFTER_MODIFIER_GPIO_Port;
  config3.modifier_pin = SHIFTER_MODIFIER_Pin;
  config3.settings = &wheel_settings->shifter.content;
  if (hShifter.INIT(&hShifter, &config3) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
}

static void init_motor_driver() {
  MotorDriver_ConfigHandleTypeDef config1 = {0};
  config1.L_EN_pin = PWM_L_EN_Pin;
  config1.R_EN_pin = PWM_R_EN_Pin;
  config1.L_EN_port = PWM_L_EN_GPIO_Port;
  config1.R_EN_port = PWM_R_EN_GPIO_Port;
  config1.pwm_timer = &htim1;
  config1.left_channel = TIM_CHANNEL_1;
  config1.right_channel = TIM_CHANNEL_2;
  config1.left_compareRegister = &TIM1->CCR1;
  config1.right_compareRegister = &TIM1->CCR2;
  if (hbts7960.INIT(&hbts7960, &config1) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }

  Actuator_ConfigHandleTypeDef config2 = {0};
  config2.hMotorDriver = &hbts7960;
  config2.settings = &wheel_settings->force.content;
  if (hActuator.INIT(&hActuator, &config2) == WHEEL_ERROR) {
    WHEEL_LOG_ERROR(WHEEL_ERR_INIT);
  }
}

// No micros() source: the library times its effects in whole milliseconds.
// Its send callback is registered later, by usb_init()
static void init_ffb_library() {
  hFFB = ffb_create(FFB_AXIS_COUNT, HAL_GetTick, NULL);
  // USB and the control loop cannot run without it
  if (hFFB == NULL) {
    WHEEL_FATAL_ERROR(WHEEL_ERR_FFB);
  }
}

// The travel between the two endstops: the mechanical one less the margin of
// the settings at each end. Only the effects use it: the steering axis sent to
// the host always covers the full travel
static float endstop_rotation_deg() {
  uint16_t margin = wheel_settings->steering.content.endstop_margin_deg;
  return (float)(MAX_ROTATION_DEG - 2 * margin);
}

// Created once: the rotation is taken from the settings of this start. The
// intensities come from apply_settings()
static ffb_axis_local_t* create_local_effects() {
  ffb_axis_local_config_t local_effects_config = {0};
  ffb_axis_local_config_default(&local_effects_config);
  local_effects_config.degrees_of_rotation = endstop_rotation_deg();
  return ffb_axis_local_create(&local_effects_config);
}

// The _ex variant takes the low-pass filters of the speed and the
// acceleration. Created once: they are the settings of this start
static ffb_metrics_t* create_metrics_helper() {
  const wheel_file_settings_steering_t *steering = &wheel_settings->steering.content;
  return ffb_metrics_create_ex(endstop_rotation_deg(), CONTROL_LOOP_RATE_HZ,
                               steering->speed_filter_hz, steering->speed_filter_q,
                               steering->accel_filter_hz, steering->accel_filter_q);
}

// Gives the ffb library the live values of the settings: it keeps copies, not
// pointers. Once the ffb objects exist, then after each change of the settings
// (wheel_command.c). The effects already running take a new filter at once
static void apply_settings() {
  const wheel_file_settings_ffb_t *ffb = &wheel_settings->ffb.content;
  ffb_axis_local_set_intensities(local_effects, ffb->endstop_strength,
                                 ffb->damper_intensity, ffb->friction_intensity,
                                 ffb->inertia_intensity);
  ffb_axis_local_set_idle_spring(local_effects, ffb->idle_spring_strength);

  // Only the filters of the settings change in the preset
  ffb_effect_filter_preset_t old_preset = {0};
  ffb_get_filter_preset(hFFB, FFB_FILTER_PROFILE, &old_preset);
  ffb_effect_filter_preset_t filter_preset = old_preset;
  filter_preset.constant_freq = ffb->constant_filter_hz;
  filter_preset.constant_q = ffb->constant_filter_q;
  filter_preset.damper_freq = ffb->damper_filter_hz;
  filter_preset.friction_freq = ffb->friction_filter_hz;
  filter_preset.inertia_freq = ffb->inertia_filter_hz;
  ffb_set_filter_preset(hFFB, FFB_FILTER_PROFILE, &filter_preset);

  // Rebuilding the filters of a type clears the state of its running effects
  // (calcBiquad() in ffb_biquad.cpp): only the types whose filter changed
  if (filter_preset.constant_freq != old_preset.constant_freq || filter_preset.constant_q
      != old_preset.constant_q) {
    ffb_update_filters_for_type(hFFB, FFB_TYPE_CONSTANT);
  }
  if (filter_preset.damper_freq != old_preset.damper_freq) {
    ffb_update_filters_for_type(hFFB, FFB_TYPE_DAMPER);
  }
  if (filter_preset.friction_freq != old_preset.friction_freq) {
    ffb_update_filters_for_type(hFFB, FFB_TYPE_FRICTION);
  }
  if (filter_preset.inertia_freq != old_preset.inertia_freq) {
    ffb_update_filters_for_type(hFFB, FFB_TYPE_INERTIA);
  }
}

#endif /* WHEEL_INIT_H_ */
