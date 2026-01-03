/*
 * startupWheel.c
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#include "steeringwheel.h"
#include "ffb_library.h"
#include <math.h>

Wheel_HandleTypeDef wheel;

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
USB_HID_HandleTypeDef hUsbHidPid;

static void init_wheel_handle();
static void init_buttons();
static void init_sensor();
static void init_analog();
static void init_motor_driver();
static void configure_software_exti();
static void register_initialization_error();

static Wheel_Status wheel_axis_calibration();

#define CALIBRATION_FORCE 150
#define CALIBRATION_MAX_TRIES 3
#define STEERING_RESISTANCE_START 30000

int32_t forces[2];

void wheel_startup() {
	/* INIT */
	init_wheel_handle();
	init_analog();
	init_buttons();
	init_sensor();
	init_motor_driver();
	configure_software_exti();
	app_usb_hid_init(&hUsbHidPid);

	/* START MODULES */
	Sensor_HandleTypeDef *sensor = wheel.hSensor;
	Analog_HandleTypeDef *analog = wheel.hPedals->hw_analog;
	Buttons_HandleTypeDef *buttons = wheel.hButtons;
	Magnetometer_HandleTypeDef *magnetometer = wheel.hSensor->hw_magnetometer;
	if (analog->Start_CONTINUOUS_SCAN_DMA(analog) == WHEEL_ERROR) {
		register_initialization_error();
	}
	if (buttons->Start_TIM_POLL(buttons) == WHEEL_ERROR) {
		register_initialization_error();
	}
	if (magnetometer->Start_TIM_POLL(magnetometer) == WHEEL_ERROR) {
		register_initialization_error();
	}
	if (ffb_init() == WHEEL_ERROR) {
		register_initialization_error();
	}
	if (app_usb_start() == WHEEL_ERROR) {
		register_initialization_error();
	}

	HAL_Delay(3000); // waiting for a bit won't do any harm

	// try calibration until succeeds or the max attempts number is reached
	uint8_t calibration_tries = 0;
	while (wheel_axis_calibration() == WHEEL_ERROR) {
		HAL_Delay(3000);
		calibration_tries++;
		if (calibration_tries > CALIBRATION_MAX_TRIES) {
			Error_Handler();
		}
	}

	/* temporary code for force feedback testing */
	forces[0] = 0;
	forces[1] = 0;
	const int16_t max = MOTOR_MAX_FORCE;
	const int16_t min = MOTOR_MIN_FORCE;
	const int16_t range = STEERING_RESISTANCE_START;
	const int16_t normalizer = (0x7FFF - STEERING_RESISTANCE_START)
			* (70.f / 100.f);
	while (1) {
		HAL_Delay(10);
		ffb_updateAxis(sensor->virtual_axis);
		ffb_getForces(forces);
		int16_t axis = wheel.hSensor->virtual_axis;
		float force_coef;
		if (axis < -range) {
			force_coef = ((int32_t) (axis + range) * max) / -normalizer;
			forces[1] = (force_coef > max) ? max : (int16_t) force_coef;
		} else if (axis > range) {
			force_coef = ((int32_t) (axis - range) * min) / normalizer;
			forces[1] = (force_coef < min) ? min : (int16_t) force_coef;
		}
		if (wheel.hActuator->Apply_Force(wheel.hActuator, (int16_t) forces[1])
				== WHEEL_ERROR) {
			wheel.wheel_error_count++;
		}
	}
}

/* Perform an automatic calibration like in the original g29.
 * Purpose : calculate the full range of the wheel by determining min and max
 * to find out the middle and the scaling coefficient
 * Functioning : go left until hit a wall, in the meantime do -> "min = current_pos"
 * then repeat in the opposite direction
 */
static Wheel_Status wheel_axis_calibration() {
	Sensor_HandleTypeDef *sensor = wheel.hSensor;
	int16_t acceleration = 0;

	// in the left direction
	int32_t previous_min = sensor->steering_pos;
	wheel.hActuator->Apply_Force(wheel.hActuator, -CALIBRATION_FORCE); // apply force
	HAL_Delay(50); // A : appropriate delay for motors to start working
	acceleration = sensor->steering_pos - previous_min;
	while (abs(acceleration) > 150) { // acceleration threshold
		// B : finding out if current position is the farthest
		if (sensor->steering_pos < sensor->min) {
			sensor->min = sensor->steering_pos;
		}
		// C : calculating acceleration
		previous_min = sensor->steering_pos;
		HAL_Delay(10); // a delay is required
		acceleration = sensor->steering_pos - previous_min;
	}

	// in the right direction
	int32_t previous_max = sensor->steering_pos;
	wheel.hActuator->Apply_Force(wheel.hActuator, CALIBRATION_FORCE);
	HAL_Delay(50); // A
	acceleration = sensor->steering_pos - previous_max;
	while (abs(acceleration) > 150) { // acceleration threshold
		// B
		if (sensor->steering_pos > sensor->max) {
			sensor->max = sensor->steering_pos;
		}
		// C
		previous_max = sensor->steering_pos;
		HAL_Delay(10);
		acceleration = sensor->steering_pos - previous_max;
	}
	wheel.hActuator->Apply_Force(wheel.hActuator, 0);

	sensor->axis_scale = (float) (0x7FFF) / (sensor->distance / 2);
	// In testing, the range is ~64069
	if (sensor->distance < 63500) {
		// notify the caller that something went wrong
		return WHEEL_ERROR;
	} else {
		return WHEEL_OK;
	}
}

/* 		INITIALIZATION FUNCTIONS		 */
static void init_wheel_handle() {
	wheel.wheel_error_count = 0;
	wheel.hButtons = &hButtons;
	wheel.hSensor = &hSensor;
	wheel.hPedals = &hPedals;
	wheel.hShifter = &hShifter;
	wheel.hUsbHid = &hUsbHidPid;
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

static void configure_software_exti() {
	wheel.hSwit.usb_send_report_swit_pin = SWIT_0_Pin;
	wheel.hSwit.usb_process_data_pin = SWIT_1_Pin;
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
	if (GPIO_Pin == wheel.hSwit.usb_send_report_swit_pin) {
		app_usb_hid_send_report();
	}
	if (GPIO_Pin == wheel.hSwit.usb_process_data_pin) {
		app_usb_start_deferred_processing();
	}
}

// ADC callbacks not used because ADC fills the values in continuous scan mode,
// paired up with DMA, meaning that we never have to worry about it.
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	UNUSED(hadc);
}

// Called at the end of a transfer to process the raw data received by the sensor
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (wheel.hSensor->hw_magnetometer->hspi->Instance == hspi->Instance) {
		wheel.hSensor->hw_magnetometer->TxRxDone_CB(
				wheel.hSensor->hw_magnetometer);
		wheel.hSensor->Update(wheel.hSensor);
	}
}

// 1. Used to start, periodically, the transmission with the magnetometer (steering) (IMPORTANT)
// 2. Used to periodically read the buttons' state (for debouncing) (IMPORTANT)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (wheel.hSensor->hw_magnetometer->htim->Instance == htim->Instance) {
		wheel.hSensor->hw_magnetometer->TransmitRecieve_DMA(
				wheel.hSensor->hw_magnetometer);
	}
	if (wheel.hButtons->htim->Instance == htim->Instance) {
		wheel.hButtons->hw_buttons->ReadState(wheel.hButtons->hw_buttons);
		wheel.hButtons->Update(wheel.hButtons);
	}
}
