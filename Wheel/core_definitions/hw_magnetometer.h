/*
 MIT License

 Copyright (c) 2025 Log1cFX

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/*
 * hw_magnetometer.h
 *
 *  Created on: Sep 22, 2024
 *      Author: raffi
 */

#ifndef CORE_DEFINITIONS_HW_MAGNETOMETER_H_
#define CORE_DEFINITIONS_HW_MAGNETOMETER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

typedef struct {
	uint16_t SS_pin;
	GPIO_TypeDef *SS_port;
	SPI_HandleTypeDef *hspi;
	TIM_HandleTypeDef *htim; // This timer is used to periodically get information from the sensor
} Magnetometer_ConfigHandleTypeDef;

typedef struct _Magnetometer_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Magnetometer_HandleTypeDef *sensor,
			Magnetometer_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Magnetometer_HandleTypeDef *sensor);
	Wheel_Status (*Start_TIM_POLL)(struct _Magnetometer_HandleTypeDef *sensor);
	Wheel_Status (*Stop)(struct _Magnetometer_HandleTypeDef *sensor);
	// TransmitRecieve_DMA should be called by the timer (from its callback called by HAL)
	Wheel_Status (*TransmitRecieve_DMA)(
			struct _Magnetometer_HandleTypeDef *sensor);
	// This should be called when receiving the TxRxDone callback from SPI
	Wheel_Status (*TxRxDone_CB)(struct _Magnetometer_HandleTypeDef *sensor);

	uint8_t SPI_Rx_buffer[8];
	uint8_t SPI_Tx_buffer[8];
	uint16_t alpha; // this is the value that we want
	uint8_t transfer_is_done;
	uint16_t err_packets_cnt; // Either missed, corrupted or wrong packets
	uint8_t diagnostic_bits;
	uint8_t roll_cnt;

	uint16_t SS_pin; // Slave Select pin
	GPIO_TypeDef *SS_port; // Slave Select port
	SPI_HandleTypeDef *hspi;
	TIM_HandleTypeDef *htim; // TIM handle to work with DMA for timed Transfers
} Magnetometer_HandleTypeDef;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_HW_MAGNETOMETER_H_ */
