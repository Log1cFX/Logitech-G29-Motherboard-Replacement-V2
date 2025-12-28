/*
 * startupWheel.c
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#include "common_types.h"
#include "steeringwheel.h"

Wheel_HandleTypeDef wheel;

extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

extern DigitalInput_HandleTypeDef hG29Buttons;
extern Buttons_HandleTypeDef hButtons;
extern Magnetometer_HandleTypeDef hmlx90363;
extern Sensor_HandleTypeDef hSensor;
extern Analog_HandleTypeDef hAnalog;
extern Pedals_HandleTypeDef hPedals;
extern Shifter_HandleTypeDef hShifter;
USB_HID_HandleTypeDef hUsbHidPid;

static void init_buttons();
static void init_sensor();
static void init_analog();
static void configure_software_exti();
static void init_wheel_handle();

int16_t out[2];
int16_t in[2];

void wheel_startup() {
	/* INIT */
	init_analog();
	init_buttons();
	init_sensor();
	configure_software_exti();
	app_usb_hid_init(&hUsbHidPid);
	init_wheel_handle();

	/* START MODULES */
	wheel.hPedals->hw_analog->Start_CONTINIOUS_SCAN_DMA(
			wheel.hPedals->hw_analog);
	wheel.hButtons->Start_TIM(wheel.hButtons);
	wheel.hSensor->hw_magnetometer->Start_TIM(wheel.hSensor->hw_magnetometer);

	app_usb_start();

	/* temporary code for live calibration and force feedback testing */
	wheel.hSensor->min = 1;
	wheel.hSensor->max = 1;


	while (1) {
		if (wheel.hSensor->steering_pos < wheel.hSensor->min) {
			wheel.hSensor->min = wheel.hSensor->steering_pos;
		}
		if (wheel.hSensor->steering_pos > wheel.hSensor->max) {
			wheel.hSensor->max = wheel.hSensor->steering_pos;
		}
		wheel.hSensor->axis_scale = (float) (0xFFFF / 2)
				/ (wheel.hSensor->distance / 2);
		HAL_Delay(100);
		in[0] = wheel.hSensor->virtual_axis;
//		FfbGetFeedbackValue(in, out);
	}
}

/* 		INITIALIZATION FUNCTIONS		 */
static void init_buttons() {
	DigitalInput_ConfigHandleTypeDef config1 = { 0 };
	config1.buttons_port = GPIOC;
	config1.clk_pin = BUTTON_CLK_Pin;
	config1.lock_pin = BUTTON_LOCK_Pin;
	config1.in_pin = BUTTON_IN_Pin;
	if (hG29Buttons.INIT(&hG29Buttons, &config1) == WHEEL_ERROR) {
		Error_Handler();
	}

	Buttons_ConfigHandleTypeDef config2 = { 0 };
	config2.htim = &htim3;
	config2.hw_buttons = &hG29Buttons;
	if (hButtons.INIT(&hButtons, &config2) == WHEEL_ERROR) {
		Error_Handler();
	}
}

static void init_sensor() {
	Magnetometer_ConfigHandleTypeDef config1 = { 0 };
	config1.hspi = &hspi2;
	config1.htim = &htim4;
	config1.SS_port = SPI2_SS_GPIO_Port;
	config1.SS_pin = SPI2_SS_Pin;
	if (hmlx90363.INIT(&hmlx90363, &config1) == WHEEL_ERROR) {
		Error_Handler();
	}

	Sensor_ConfigHandleTypeDef config2 = { 0 };
	config2.hw_magnetometer = &hmlx90363;
	if (hSensor.INIT(&hSensor, &config2) == WHEEL_ERROR) {
		Error_Handler();
	}
}

static void init_analog() {
	Analog_ConfigHandleTypeDef config1 = { 0 };
	config1.hadc = &hadc1;
	hAnalog.INIT(&hAnalog, &config1);

	Pedals_ConfigHandleTypeDef config2 = { 0 };
	config2.hw_analog = &hAnalog;
	hPedals.INIT(&hPedals, &config2);

	Shifter_ConfigHandleTypeDef config3 = { 0 };
	config3.hw_analog = &hAnalog;
	config3.modifier_port = SHIFTER_MODIFIER_GPIO_Port;
	config3.modifier_pin = SHIFTER_MODIFIER_Pin;
	hShifter.INIT(&hShifter, &config3);
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

static void init_wheel_handle() {
	wheel.hButtons = &hButtons;
	wheel.hSensor = &hSensor;
	wheel.hPedals = &hPedals;
	wheel.hShifter = &hShifter;
	wheel.hUsbHid = &hUsbHidPid;
}

/* 		APPLICATION SPECIFIC FUNCTIONS 		*/
Wheel_Status wheel_get_all_component_states() {
	if (wheel.hPedals->hw_analog->hadc == 0) {
		return WHEEL_ERROR;
	}
	if (wheel.hButtons->hw_buttons->buttons_port == 0
			|| wheel.hButtons->htim == 0) {
		return WHEEL_ERROR;
	}
	if (wheel.hSensor->hw_magnetometer->hspi == 0) {
		return WHEEL_ERROR;
	}
	wheel.hButtons->GetState(wheel.hButtons);
	wheel.hPedals->GetState(wheel.hPedals);
	wheel.hSensor->GetAxis(wheel.hSensor);
	wheel.hShifter->GetSpeed(wheel.hShifter);
	return WHEEL_OK;
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
