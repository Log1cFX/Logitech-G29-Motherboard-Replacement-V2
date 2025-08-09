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
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

extern DigitalInput_HandleTypeDef hG29Buttons;
extern Buttons_HandleTypeDef hButtons;
extern Magnetometer_HandleTypeDef hmlx90363;
extern Sensor_HandleTypeDef hSensor;
extern Analog_HandleTypeDef hAnalog;
extern Pedals_HandleTypeDef hPedals;
USB_HID_HandleTypeDef hUsbHid;

static void init_buttons();
static void init_sensor();
static void init_analog();
static void configure_software_exti();
static void init_wheel_handle();

uint32_t time_ms = 0;
uint8_t temp = 0;
uint32_t test_time_ms;
uint32_t old_time_ms;

void wheel_startup() {
	init_analog();
	init_buttons();
	init_sensor();
	configure_software_exti();
	app_usb_hid_init(&hUsbHid);
	init_wheel_handle();

	wheel.hPedals->hw_analog->Start_CONTINIOUS_SCAN_DMA(
			wheel.hPedals->hw_analog);
	wheel.hButtons->Start_TIM(wheel.hButtons);
	wheel.hSensor->hw_magnetometer->Start_TIM(wheel.hSensor->hw_magnetometer);

	app_usb_hid_start();

	while (1) {
		__WFI();
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
	config2.htim = &htim4;
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
	wheel.hPedals = &hPedals;
	wheel.hSensor = &hSensor;
	wheel.hUsbHid = &hUsbHid;
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
	return WHEEL_OK;
}

/* 			LOW_LEVEL CALLBACKS		 	*/
// The custom software interrupt implementation using the EXTI line callbacks
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == wheel.hSwit.usb_send_report_swit_pin) {
		app_usb_hid_send_report();
	}
	if (GPIO_Pin == wheel.hSwit.usb_process_data_pin) {
		app_usb_hid_deferred_processing();
	}
}

/* ADC CALLBACK */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	UNUSED(hadc);
}

/* SPI CALLBACK */
// Called at the end of a transfer to process the raw data received by the sensor
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (wheel.hSensor->hw_magnetometer->hspi->Instance == hspi->Instance) {
		wheel.hSensor->hw_magnetometer->TxRxDone_CB(
				wheel.hSensor->hw_magnetometer);
		wheel.hSensor->Update(wheel.hSensor);
		if(temp==1){
			test_time_ms = time_ms - old_time_ms;
			temp=2;
		}
	}
}

/* TIM CALLBACK */
// 1. Used to start, periodically, the transmission with the sensor
// 2. Used to periodically read the buttons state
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (wheel.hSensor->hw_magnetometer->htim->Instance == htim->Instance) {
		wheel.hSensor->hw_magnetometer->TransmitRecieve_DMA(
				wheel.hSensor->hw_magnetometer);
		if(temp == 0){
			temp = 1;
			old_time_ms = time_ms;
		}
	}
	if (wheel.hButtons->htim->Instance == htim->Instance) {
		wheel.hButtons->hw_buttons->ReadState(wheel.hButtons->hw_buttons);
		wheel.hButtons->Update(wheel.hButtons);
		time_ms++;
	}
}
