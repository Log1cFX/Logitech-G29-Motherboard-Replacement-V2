/*
 * hw_analog_input.h
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#ifndef INC_HW_ANALOG_INPUT_H_
#define INC_HW_ANALOG_INPUT_H_

#include "common_types.h"

#define ANALOG_INPUT_NUM 5
#define PEDALS_NUM 3
#define PEDALS_IDX 0
#define SHIFTER_IDX 3

typedef struct {
	ADC_HandleTypeDef *hadc;
} Analog_ConfigHandleTypeDef;

typedef struct _Analog_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Analog_HandleTypeDef *analog,
			Analog_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Analog_HandleTypeDef *analog);
	Wheel_Status (*Start_CONTINIOUS_SCAN_DMA)(
			struct _Analog_HandleTypeDef *analog);
	Wheel_Status (*Stop)(struct _Analog_HandleTypeDef *analog);

	uint32_t axis[ANALOG_INPUT_NUM];

	ADC_HandleTypeDef *hadc;
} Analog_HandleTypeDef;

typedef struct _Pedals_ConfigHandleTypeDef {
	Analog_HandleTypeDef *hw_analog;
} Pedals_ConfigHandleTypeDef;

typedef struct _Pedals_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Pedals_HandleTypeDef *analog,
			Pedals_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Pedals_HandleTypeDef *analog);
	Wheel_Status (*GetState)(struct _Pedals_HandleTypeDef *analog);

	uint8_t clutch;
	uint8_t brake;
	uint8_t throtle;

	Analog_HandleTypeDef *hw_analog;
} Pedals_HandleTypeDef;

#endif /* INC_HW_ANALOG_INPUT_H_ */
