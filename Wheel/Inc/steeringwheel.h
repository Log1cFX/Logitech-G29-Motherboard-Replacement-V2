/*
 * startupWheel.h
 *
 *  Created on: Jul 7, 2025
 *      Author: raffi
 */

#ifndef INC_STEERINGWHEEL_H_
#define INC_STEERINGWHEEL_H_

#include "common_types.h"
#include "hw_digital_input.h"
#include "sw_buttons.h"
#include "hw_magnetometer.h"
#include "sw_sensor.h"
#include "hw_analog_input.h"
#include "sw_shifter.h"
#include "app_usb_hid.h"

#define MAX_PEDALS 3

struct _USB_HID_HandleTypeDef;

typedef struct {
	uint16_t usb_send_report_swit_pin;
	uint16_t usb_process_data_pin;
	uint16_t swit_pin_2; // Unused
}Wheel_SWIT_HandleTypeDef;

typedef struct {
	Buttons_HandleTypeDef *hButtons;
	Sensor_HandleTypeDef *hSensor;
	Pedals_HandleTypeDef *hPedals;
	Shifter_HandleTypeDef *hShifter;
	Wheel_SWIT_HandleTypeDef hSwit;
	struct _USB_HID_HandleTypeDef *hUsbHid;
} Wheel_HandleTypeDef;

void wheel_startup();
Wheel_Status wheel_get_all_component_states();

#endif /* INC_STEERINGWHEEL_H_ */
