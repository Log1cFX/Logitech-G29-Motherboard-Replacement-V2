/*
 * MLX90363.h
 *
 *  Created on: Sep 22, 2024
 *      Author: raffi
 */

#ifndef INC_MLX90363_H_
#define INC_MLX90363_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

#define GET1_OPCODE 				0x13
#define NOP_OPCODE					0xD0
#define NULL_DATA					0x00
#define NOP_KEY						0xAA
#define GET_TIME_OUT 				0xFF

typedef struct {
	uint16_t SS_pin;
	GPIO_TypeDef *SS_port;
	SPI_HandleTypeDef *hspi;
	TIM_HandleTypeDef *htim;
} Magnetometer_ConfigHandleTypeDef;

typedef struct _Magnetometer_HandleTypeDef {
	Wheel_Status (*INIT)(struct _Magnetometer_HandleTypeDef *sensor,
			Magnetometer_ConfigHandleTypeDef *config);
	Wheel_Status (*DeINIT)(struct _Magnetometer_HandleTypeDef *sensor);
	Wheel_Status (*Start_TIM)(struct _Magnetometer_HandleTypeDef *sensor);
	Wheel_Status (*Stop)(struct _Magnetometer_HandleTypeDef *sensor);
	Wheel_Status (*TransmitRecieve_DMA)(
			struct _Magnetometer_HandleTypeDef *sensor);
	Wheel_Status (*TxRxDone_CB)(struct _Magnetometer_HandleTypeDef *sensor);

	uint8_t SPI_Rx_buffer[8];
	uint8_t SPI_Tx_buffer[8];
	uint16_t alpha;
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

#endif /* INC_MLX90363_H_ */
