/*
 * startupWheel.c
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#include "wheel_def.h"
#include "usb_processing.h"
#include "ffb/ffb_c.h"
#include "ffb/ffb_metrics_c.h"
#include "ffb/ffb_axis_local_c.h"

#include <stdlib.h>

Wheel_HandleTypeDef wheel;
ffb_lib_t *hFFB;

extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

extern DigitalInput_HandleTypeDef hG29Buttons;
extern Buttons_HandleTypeDef hButtons;
extern Magnetometer_HandleTypeDef hmlx90363;
extern Sensor_HandleTypeDef hSensor;
extern Analog_HandleTypeDef hAnalog;
extern Pedals_HandleTypeDef hPedals;
extern Shifter_HandleTypeDef hShifter;
extern MotorDriver_HandleTypeDef hMotorDriver;
extern Actuator_HandleTypeDef hActuator;

static void init_wheel_handle();
static void init_buttons();
static void init_sensor();
static void init_analog();
static void init_motor_driver();
static void init_usb();
static void init_ffb_library();
static void configure_software_exti();
static void register_initialization_error();

static Wheel_Status wheel_axis_calibration();
static void wheel_recenter();

void wheel_startup() {
	/* INIT */
	init_analog();
	init_buttons();
	init_sensor();
	init_motor_driver();
	configure_software_exti();
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
	init_usb();

	// try calibration until succeeds or the max attempts number is reached
	uint8_t calibration_tries = 0;
	while (wheel_axis_calibration() == WHEEL_ERROR) {
		HAL_Delay(2000);
		calibration_tries++;
		if (calibration_tries >= CALIBRATION_MAX_TRIES) {
			register_initialization_error();
		}
	}
	wheel_recenter();

	// set up the metrics helper
	const float rot_deg = MAX_ROTATION_DEG - (ENDSTOP_DEG_OFFSET * 2);
	ffb_metrics_t *metrics = ffb_metrics_create(rot_deg, 1000.0f);
	// set up the local effects
	ffb_axis_local_config_t local_effects_config = { 0 };
	ffb_axis_local_config_default(&local_effects_config);
	local_effects_config.degrees_of_rotation = rot_deg;
	local_effects_config.endstop_strength = 250;
	local_effects_config.idle_spring_strength = 255;
	local_effects_config.damper_intensity = 0;
	ffb_axis_local_t *local_effects = ffb_axis_local_create(
			&local_effects_config);

	// force variables
	static int32_t local_force = 0;
	static int32_t host_force = 0;
	static int32_t total_force = 0;
	static int16_t end_force = 0;

	uint32_t last_executed_time = HAL_GetTick();
	uint32_t current_time = HAL_GetTick();
	while (1) {
		// mendatory tinyusb's task
		tud_task();
		current_time = HAL_GetTick();

		// execute every 1ms
		if (current_time - last_executed_time >= 1) {
			last_executed_time = current_time;

			// if no game is controlling the ffb, configure damper effect
			if (!ffb_is_active(hFFB)) {
				ffb_axis_local_set_intensities(local_effects, 250, 255, 0, 0);
			} else {
				ffb_axis_local_set_intensities(local_effects, 250, 0, 0, 0);
			}

			// compute current degrees and update the axis state
			float degrees = wheel.hSensor->virtual_axis
					* (float) (MAX_ROTATION_DEG / 2) / (float) INT16_MAX;
			ffb_axis_state_t st = ffb_metrics_update(metrics, degrees);

			// compute forces
			ffb_set_axis_state_s(hFFB, 0, &st);
			ffb_calculate(hFFB);
			host_force = ffb_get_axis_torque(hFFB, 0);
			local_force = ffb_axis_local_compute(local_effects, &st, degrees,
					ffb_is_active(hFFB));
			total_force = host_force + local_force;

			// remap and clamp the final force
			end_force = remapf(INT16_MIN, INT16_MAX, total_force,
			MOTOR_MIN_FORCE, MOTOR_MAX_FORCE);
			end_force = clamp(end_force, MOTOR_MIN_FORCE, MOTOR_MAX_FORCE);

			// apply the force on the motor
			if (wheel.hActuator->Apply_Force(wheel.hActuator,
					(int16_t) end_force) == WHEEL_ERROR) {
				wheel.wheel_error_count++;
			}
		}
	}
}

/* One calibration sweep: push with `force` until the wheel stalls
 * (acceleration drops below threshold), tracking the extremum.
 * dir = +1 goes right, tracking a maximum (pos > extremum).
 * dir = -1 goes left,  tracking a minimum (pos < extremum). */
static void wheel_calib_sweep(Sensor_HandleTypeDef *sensor, int16_t force,
		int32_t *extremum, int dir) {
	int32_t previous = sensor->steering_pos;
	wheel.hActuator->Apply_Force(wheel.hActuator, force);
	HAL_Delay(40); // A: let the motor start
	int16_t acceleration = sensor->steering_pos - previous;
	while (abs(acceleration) > 150) {
		// B: finding out if current position is the farthest
		if (dir * sensor->steering_pos > dir * *extremum) {
			*extremum = sensor->steering_pos;
		}
		// C: calculating acceleration
		previous = sensor->steering_pos;
		HAL_Delay(10);
		acceleration = sensor->steering_pos - previous;
	}
}

static Wheel_Status wheel_axis_calibration() {
	Sensor_HandleTypeDef *sensor = wheel.hSensor;

	wheel_calib_sweep(sensor, -CALIBRATION_FORCE, &sensor->min, -1); // left
	wheel_calib_sweep(sensor, CALIBRATION_FORCE, &sensor->max, +1); // right
	wheel.hActuator->Apply_Force(wheel.hActuator, 0);

	sensor->axis_scale = (float) (0x7FFF) / (sensor->distance / 2);
	// In testing, the range is ~64069
	return (sensor->distance < 63750) ? WHEEL_ERROR : WHEEL_OK;
}

static void wheel_recenter() {
	int8_t sign = (wheel.hSensor->virtual_axis > 0) ? 1 : -1;
	while (abs(wheel.hSensor->virtual_axis) > 150) {
		wheel.hActuator->Apply_Force(wheel.hActuator,
				CALIBRATION_FORCE * -sign);
	}
	wheel.hActuator->Apply_Force(wheel.hActuator, 0);
}

/* 		INITIALIZATION FUNCTIONS		 */
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
	DigitalInput_ConfigHandleTypeDef config1 = { 0 };
	config1.buttons_port = GPIOC;
	config1.clk_pin = BUTTON_CLK_Pin;
	config1.lock_pin = BUTTON_LOCK_Pin;
	config1.in_pin = BUTTON_IN_Pin;
	if (hG29Buttons.INIT(&hG29Buttons, &config1) == WHEEL_ERROR) {
		register_initialization_error();
	}

	Buttons_ConfigHandleTypeDef config2 = { 0 };
	config2.htim = &htim3;
	config2.hw_buttons = &hG29Buttons;
	if (hButtons.INIT(&hButtons, &config2) == WHEEL_ERROR) {
		register_initialization_error();
	}
}

static void init_sensor() {
	Magnetometer_ConfigHandleTypeDef config1 = { 0 };
	config1.hspi = &hspi2;
	config1.htim = &htim4;
	config1.SS_port = SPI2_SS_GPIO_Port;
	config1.SS_pin = SPI2_SS_Pin;
	if (hmlx90363.INIT(&hmlx90363, &config1) == WHEEL_ERROR) {
		register_initialization_error();
	}

	Sensor_ConfigHandleTypeDef config2 = { 0 };
	config2.hw_magnetometer = &hmlx90363;
	if (hSensor.INIT(&hSensor, &config2) == WHEEL_ERROR) {
		register_initialization_error();
	}
}

static void init_analog() {
	Analog_ConfigHandleTypeDef config1 = { 0 };
	config1.hadc = &hadc1;
	if (hAnalog.INIT(&hAnalog, &config1) == WHEEL_ERROR) {
		register_initialization_error();
	}

	Pedals_ConfigHandleTypeDef config2 = { 0 };
	config2.hw_analog = &hAnalog;
	if (hPedals.INIT(&hPedals, &config2) == WHEEL_ERROR) {
		register_initialization_error();
	}

	Shifter_ConfigHandleTypeDef config3 = { 0 };
	config3.hw_analog = &hAnalog;
	config3.modifier_port = SHIFTER_MODIFIER_GPIO_Port;
	config3.modifier_pin = SHIFTER_MODIFIER_Pin;
	if (hShifter.INIT(&hShifter, &config3) == WHEEL_ERROR) {
		register_initialization_error();
	}
}

static void init_motor_driver() {
	MotorDriver_ConfigHandleTypeDef config1 = { 0 };
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

	Actuator_ConfigHandleTypeDef config2 = { 0 };
	config2.hMotorDriver = &hMotorDriver;
	if (hActuator.INIT(&hActuator, &config2) == WHEEL_ERROR) {
		register_initialization_error();
	}
}

static void init_usb() {
// wait till device gets enumerate device
	while (!tud_ready()) {
		tud_task();
	}
	set_hid_driver_state(0);
// send a report, when it gets sent and tud_hid_report_complete_cb fires
// we'll know the driver has been initialized on the host
	uint8_t empty_buf[REPORT_SIZE] = { 0 };
	tud_hid_report(JOYSTICK_REPORT_ID, empty_buf, REPORT_SIZE);
	while (!hid_driver_ready()) {
		tud_task();
	}
}

static void init_ffb_library() {
	hFFB = ffb_create(1, HAL_GetTick, get_faketime_micros);
	ffb_set_send_report_callback(hFFB, ffb_send_report_cb);
}

static void configure_software_exti() {
	for (uint8_t i = 0; i < 3; i++) {
		CLEAR_BIT(EXTI->RTSR, (0x1UL << i));  // Clear rising edge
		CLEAR_BIT(EXTI->FTSR, (0x1UL << i));  // Clear falling edge
		SET_BIT(EXTI->IMR, (0x1UL << i));  // Already enabled by CubeMX
		SET_BIT(EXTI->PR, (0x1UL << i));  // Clear any pending interrupts
	}
}

static void register_initialization_error() {
#ifdef DEBUG
	Error_Handler();
#endif
#ifdef RELEASE
	wheel.wheel_error_count++;
#endif
}

/* 		APPLICATION SPECIFIC FUNCTIONS 		*/
Wheel_Status wheel_get_all_component_states() {
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

/*		HARDWARE CALLBACK FUNCTIONS		 	*/
// Custom software interrupt implementation using the EXTI line callbacks
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == SEND_REPORT_SWIT_PIN) {
		usb_send_report();
	}
}

// ADC callbacks not used because ADC fills the values in continuous scan mode,
// paired up with DMA, meaning that we never have to worry about it.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	UNUSED(hadc);
}

// Called at the end of a transfer to process the raw data received by the sensor
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (wheel.hMagnetometer->Config.hspi->Instance == hspi->Instance) {
		wheel.hMagnetometer->TxRxDone_CB(wheel.hMagnetometer);
		wheel.hSensor->Update(wheel.hSensor);
	}
}

// 1. Used to start, periodically, the transmission with the magnetometer (steering)
// 2. Used to periodically read the buttons' state (for debouncing)
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
