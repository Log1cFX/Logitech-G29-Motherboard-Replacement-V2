/*
 * sw_shifter.h
 *
 *  Created on: Aug 14, 2025
 *      Author: raffi
 */

#ifndef SRC_SW_SHIFTER_H_
#define SRC_SW_SHIFTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_analog_input.h"

typedef struct {
	uint16_t x;
	uint16_t y;
} Point;

typedef struct {
	Analog_HandleTypeDef *hw_analog;
	uint16_t modifier_pin;
	GPIO_TypeDef *modifier_port;
} Shifter_ConfigHandleTypeDef;

typedef struct _Shifter_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Shifter_HandleTypeDef *shifter,
			Shifter_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Shifter_HandleTypeDef *shifter);
	Wheel_Status (*GetSpeed)(struct _Shifter_HandleTypeDef *shifter);

	Point min;
	Point max;
	Point current_pos;
	uint8_t speed;

	uint16_t modifier_pin;
	GPIO_TypeDef *modifier_port;
	Analog_HandleTypeDef *hw_analog;
} Shifter_HandleTypeDef;

#ifdef __cplusplus
}
#endif

#endif /* SRC_SW_SHIFTER_H_ */
