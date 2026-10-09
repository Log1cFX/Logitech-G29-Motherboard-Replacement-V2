/*
 * wheel_init.h
 *
 *  Created on: 8 oct. 2026
 *      Author: raffi
 *
 *  The functions that initialize the modules of the wheel and the ffb library.
 *
 *  This is not a normal header : it is a part of steeringwheel.c that was moved
 *  out to keep that file short. It is included by steeringwheel.c and by
 *  nothing else. It uses what is defined there before the include : wheel,
 *  hFFB and register_initialization_error()
 */

#ifndef WHEEL_INIT_H_
#define WHEEL_INIT_H_

static void init_wheel_handle();
static void init_buttons();
static void init_sensor();
static void init_analog();
static void init_motor_driver();
static void init_ffb_library();
static ffb_axis_local_t* create_local_effects();
static ffb_metrics_t* create_metrics_helper();
static void init_filter_preset();

static void init_wheel_handle() {
  wheel.wheel_error_count = 0;
  wheel.hDigitalInput = &hG29Buttons;
  wheel.hButtons = &hButtons;
  wheel.hMagnetometer = &hmlx90363;
  wheel.hSensor = &hSensor;
  wheel.hAnalog = &hAnalog;
  wheel.hPedals = &hPedals;
  wheel.hShifter = &hShifter;
  wheel.hMotorDriver = &hMotorDriver;
  wheel.hActuator = &hActuator;
}

static void init_buttons() {
  DigitalInput_ConfigHandleTypeDef config1 = {0};
  config1.buttons_port = GPIOC;
  config1.clk_pin = BUTTON_CLK_Pin;
  config1.lock_pin = BUTTON_LOCK_Pin;
  config1.in_pin = BUTTON_IN_Pin;
  if (hG29Buttons.INIT(&hG29Buttons, &config1) == WHEEL_ERROR) {
    register_initialization_error();
  }

  Buttons_ConfigHandleTypeDef config2 = {0};
  config2.htim = &htim3;
  config2.hw_buttons = &hG29Buttons;
  if (hButtons.INIT(&hButtons, &config2) == WHEEL_ERROR) {
    register_initialization_error();
  }
}

static void init_sensor() {
  Magnetometer_ConfigHandleTypeDef config1 = {0};
  config1.hspi = &hspi2;
  config1.htim = &htim4;
  config1.SS_port = SPI2_SS_GPIO_Port;
  config1.SS_pin = SPI2_SS_Pin;
  if (hmlx90363.INIT(&hmlx90363, &config1) == WHEEL_ERROR) {
    register_initialization_error();
  }

  Sensor_ConfigHandleTypeDef config2 = {0};
  config2.hw_magnetometer = &hmlx90363;
  if (hSensor.INIT(&hSensor, &config2) == WHEEL_ERROR) {
    register_initialization_error();
  }
}

static void init_analog() {
  Analog_ConfigHandleTypeDef config1 = {0};
  config1.hadc = &hadc1;
  if (hAnalog.INIT(&hAnalog, &config1) == WHEEL_ERROR) {
    register_initialization_error();
  }

  Pedals_ConfigHandleTypeDef config2 = {0};
  config2.hw_analog = &hAnalog;
  if (hPedals.INIT(&hPedals, &config2) == WHEEL_ERROR) {
    register_initialization_error();
  }

  Shifter_ConfigHandleTypeDef config3 = {0};
  config3.hw_analog = &hAnalog;
  config3.modifier_port = SHIFTER_MODIFIER_GPIO_Port;
  config3.modifier_pin = SHIFTER_MODIFIER_Pin;
  if (hShifter.INIT(&hShifter, &config3) == WHEEL_ERROR) {
    register_initialization_error();
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
  if (hMotorDriver.INIT(&hMotorDriver, &config1) == WHEEL_ERROR) {
    register_initialization_error();
  }

  Actuator_ConfigHandleTypeDef config2 = {0};
  config2.hMotorDriver = &hMotorDriver;
  if (hActuator.INIT(&hActuator, &config2) == WHEEL_ERROR) {
    register_initialization_error();
  }
}

// the usb module registers itself in the library to send its reports (usb_init)
static void init_ffb_library() {
  hFFB = ffb_create(FFB_AXIS_COUNT, HAL_GetTick, NULL);
}

static ffb_axis_local_t* create_local_effects() {
  ffb_axis_local_config_t local_effects_config = {0};
  ffb_axis_local_config_default(&local_effects_config);
  local_effects_config.degrees_of_rotation = CONSTRAINED_ROTATION_DEG;
  local_effects_config.endstop_strength = LOCAL_ENDSTOP_STRENGTH;
  local_effects_config.idle_spring_strength = LOCAL_IDLE_SPRING_STRENGTH;
  local_effects_config.damper_intensity = LOCAL_DAMPER_INTENSITY;
  return ffb_axis_local_create(&local_effects_config);
}

static ffb_metrics_t* create_metrics_helper() {
  // set up the metrics helper. Use ffb_metrics_create_ex so we can lower the
  // speed/accel low-pass cutoffs below the defaults ({70,55}/{55,30}): a lower
  // cutoff (Hz) attenuates more high-frequency content from the raw encoder
  // derivatives, at the cost of slightly more phase lag. q stays Q*100.
  return ffb_metrics_create_ex(CONSTRAINED_ROTATION_DEG, CONTROL_LOOP_RATE_HZ,
  /* speed */METRICS_SPEED_FREQ_HZ,
                               METRICS_SPEED_Q,
                               /* accel */METRICS_ACCEL_FREQ_HZ,
                               METRICS_ACCEL_Q);
}

static void init_filter_preset() {
  ffb_effect_filter_preset_t filter_preset = {0};
  ffb_get_filter_preset(hFFB, FFB_FILTER_PROFILE, &filter_preset);
  filter_preset.constant_freq = FFB_CONSTANT_FILTER_FREQ_HZ;
  filter_preset.constant_q = FFB_CONSTANT_FILTER_Q;
  filter_preset.damper_freq = FFB_DAMPER_FILTER_FREQ_HZ;
  filter_preset.friction_freq = FFB_FRICTION_FILTER_FREQ_HZ;
  filter_preset.inertia_freq = FFB_INERTIA_FILTER_FREQ_HZ;
  ffb_set_filter_preset(hFFB, FFB_FILTER_PROFILE, &filter_preset);
}

#endif /* WHEEL_INIT_H_ */
