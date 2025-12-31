/*
 * startupWheel.c
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#include "steeringwheel.h"
#include "ffb_library.h"

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

int32_t forces[2];
int32_t force;

int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {

  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;

}

void wheel_startup() {
	/* INIT */
	init_wheel_handle();
	init_analog();
	init_buttons();
	init_sensor();
	init_motor_driver();
	configure_software_exti();
	app_usb_hid_init(&hUsbHidPid);

	/* from here access handles from the wheel handle */
	/* START MODULES */
	Analog_HandleTypeDef *analog = wheel.hPedals->hw_analog;
	Buttons_HandleTypeDef *buttons = wheel.hButtons;
	Magnetometer_HandleTypeDef *magnetometer = wheel.hSensor->hw_magnetometer;
	analog->Start_CONTINUOUS_SCAN_DMA(analog);
	buttons->Start_TIM(buttons);
	magnetometer->Start_TIM(magnetometer);

	if (ffb_init() == WHEEL_ERROR) {
		register_initialization_error();
	}
	app_usb_start();

	/* temporary code for live calibration and force feedback testing */
	Sensor_HandleTypeDef *sensor = wheel.hSensor;
	sensor->min = 1;
	sensor->max = 1;

	forces[0] = 0;
	forces[1] = 0;

	while (1) {
		if (sensor->steering_pos < sensor->min) {
			sensor->min = sensor->steering_pos;
		}
		if (sensor->steering_pos > sensor->max) {
			sensor->max = sensor->steering_pos;
		}
		sensor->axis_scale = (float) (0xFFFF / 2) / (sensor->distance / 2);

		HAL_Delay(10);
		ffb_updateAxis(sensor->virtual_axis);
		ffb_getForces(forces);
		force = map(forces[0], -10000, 10000, -250, 250);
		if(wheel.hActuator->Apply_Force(wheel.hActuator, (int16_t) force) == WHEEL_ERROR){
			wheel.wheel_error_count++;
		}
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
