/*
 * MLX90363.c
 *
 *  Created on: Sep 22, 2024
 *      Author: raffi
 */

#include "hw_magnetometer.h"

static Wheel_Status MLX90363_INIT(Magnetometer_HandleTypeDef *sensor,
		Magnetometer_ConfigHandleTypeDef *config);
static Wheel_Status MLX90363_DeINIT(Magnetometer_HandleTypeDef *sensor);
static Wheel_Status MLX90363_Start_TIM(Magnetometer_HandleTypeDef *sensor);
static Wheel_Status MLX90363_Stop(Magnetometer_HandleTypeDef *sensor);
static Wheel_Status MLX90363_TransmitRecieve_DMA(
		Magnetometer_HandleTypeDef *sensor);
static Wheel_Status MLX90363_TxRxDone_CB(Magnetometer_HandleTypeDef *sensor);

Magnetometer_HandleTypeDef hmlx90363 = { MLX90363_INIT, MLX90363_DeINIT,
		MLX90363_Start_TIM, MLX90363_Stop, MLX90363_TransmitRecieve_DMA,
		MLX90363_TxRxDone_CB };

static uint8_t calculate_crc(uint8_t *message);
static void set_Tx_GET1(Magnetometer_HandleTypeDef *sensor, uint8_t reset);
static void set_Tx_NOP(Magnetometer_HandleTypeDef *sensor);
static void transmit_blocking(Magnetometer_HandleTypeDef *sensor);
static void reset_roll_counter(Magnetometer_HandleTypeDef *sensor);
static uint8_t getData(Magnetometer_HandleTypeDef *sensor);

static Wheel_Status MLX90363_INIT(Magnetometer_HandleTypeDef *sensor,
		Magnetometer_ConfigHandleTypeDef *config) {
	if (config->SS_pin == 0 || config->SS_port == 0 || config->hspi == 0
			|| config->htim == 0) {
		return WHEEL_ERROR;
	}
	sensor->SS_pin = config->SS_pin;
	sensor->SS_port = config->SS_port;
	sensor->hspi = config->hspi;
	sensor->htim = config->htim;
	return WHEEL_OK;
}

static Wheel_Status MLX90363_DeINIT(Magnetometer_HandleTypeDef *sensor) {
	if (MLX90363_Stop(sensor) == WHEEL_ERROR) {
		return WHEEL_ERROR;
	}
	sensor->SS_pin = 0;
	sensor->SS_port = 0;
	sensor->hspi = 0;
	sensor->htim = 0;
	return WHEEL_OK;
}

static Wheel_Status MLX90363_Start_TIM(Magnetometer_HandleTypeDef *sensor) {
	if (sensor->htim == 0) {
		return WHEEL_ERROR;
	}
	if (sensor->hspi == 0) {
		return WHEEL_ERROR;
	}
	reset_roll_counter(sensor);
	sensor->roll_cnt = 0;
	HAL_TIM_Base_Start_IT(sensor->htim);
	return WHEEL_OK;
}

static Wheel_Status MLX90363_Stop(Magnetometer_HandleTypeDef *sensor) {
	HAL_TIM_Base_Stop_IT(sensor->htim);
	return WHEEL_OK;
}

static Wheel_Status MLX90363_TransmitRecieve_DMA(
		Magnetometer_HandleTypeDef *sensor) {
	if (sensor->hspi == 0) {
		return WHEEL_ERROR;
	}
	set_Tx_GET1(sensor, 0);
	sensor->transfer_is_done = 0;
	HAL_GPIO_WritePin(sensor->SS_port, sensor->SS_pin, 0);
	HAL_SPI_TransmitReceive_DMA(sensor->hspi, sensor->SPI_Tx_buffer,
			sensor->SPI_Rx_buffer, 8);
	return WHEEL_OK;
}

static Wheel_Status MLX90363_TxRxDone_CB(Magnetometer_HandleTypeDef *sensor) {
	HAL_GPIO_WritePin(sensor->SS_port, sensor->SS_pin, 1);
	sensor->transfer_is_done = 1;
	getData(sensor);
	return WHEEL_OK;
}

const static char cba_256_TAB[] = { 0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2,
		0xCD, 0x57, 0x78, 0x09, 0x26, 0xEB, 0xC4, 0xB5, 0x9A, 0xAE, 0x81, 0xF0,
		0xDF, 0x12, 0x3D, 0x4C, 0x63, 0xF9, 0xD6, 0xA7, 0x88, 0x45, 0x6A, 0x1B,
		0x34, 0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0, 0x91, 0xBE, 0x24, 0x0B, 0x7A,
		0x55, 0x98, 0xB7, 0xC6, 0xE9, 0xDD, 0xF2, 0x83, 0xAC, 0x61, 0x4E, 0x3F,
		0x10, 0x8A, 0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47, 0xE6, 0xC9, 0xB8,
		0x97, 0x5A, 0x75, 0x04, 0x2B, 0xB1, 0x9E, 0xEF, 0xC0, 0x0D, 0x22, 0x53,
		0x7C, 0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85, 0x1F, 0x30, 0x41,
		0x6E, 0xA3, 0x8C, 0xFD, 0xD2, 0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77,
		0x58, 0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F, 0x3B, 0x14, 0x65,
		0x4A, 0x87, 0xA8, 0xD9, 0xF6, 0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E,
		0xA1, 0xE3, 0xCC, 0xBD, 0x92, 0x5F, 0x70, 0x01, 0x2E, 0xB4, 0x9B, 0xEA,
		0xC5, 0x08, 0x27, 0x56, 0x79, 0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF,
		0x80, 0x1A, 0x35, 0x44, 0x6B, 0xA6, 0x89, 0xF8, 0xD7, 0x90, 0xBF, 0xCE,
		0xE1, 0x2C, 0x03, 0x72, 0x5D, 0xC7, 0xE8, 0x99, 0xB6, 0x7B, 0x54, 0x25,
		0x0A, 0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD, 0xDC, 0xF3, 0x69, 0x46, 0x37,
		0x18, 0xD5, 0xFA, 0x8B, 0xA4, 0x05, 0x2A, 0x5B, 0x74, 0xB9, 0x96, 0xE7,
		0xC8, 0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F, 0xAB, 0x84, 0xF5,
		0xDA, 0x17, 0x38, 0x49, 0x66, 0xFC, 0xD3, 0xA2, 0x8D, 0x40, 0x6F, 0x1E,
		0x31, 0x76, 0x59, 0x28, 0x07, 0xCA, 0xE5, 0x94, 0xBB, 0x21, 0x0E, 0x7F,
		0x50, 0x9D, 0xB2, 0xC3, 0xEC, 0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A,
		0x15, 0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42 };

static uint8_t calculate_crc(uint8_t *message) {
	// calculates the CRC using the 7 bytes of the buffer
	uint8_t crc = message[7];
	crc = 0xFF;
	crc = cba_256_TAB[message[0] ^ crc];
	crc = cba_256_TAB[message[1] ^ crc];
	crc = cba_256_TAB[message[2] ^ crc];
	crc = cba_256_TAB[message[3] ^ crc];
	crc = cba_256_TAB[message[4] ^ crc];
	crc = cba_256_TAB[message[5] ^ crc];
	crc = cba_256_TAB[message[6] ^ crc];
	crc = ~crc;
	return crc;
}

static void set_Tx_GET1(Magnetometer_HandleTypeDef *sensor, uint8_t reset) {
	sensor->SPI_Tx_buffer[0] = NULL_DATA;
	sensor->SPI_Tx_buffer[1] = reset;
	sensor->SPI_Tx_buffer[2] = GET_TIME_OUT;
	sensor->SPI_Tx_buffer[3] = GET_TIME_OUT;
	sensor->SPI_Tx_buffer[4] = NULL_DATA;
	sensor->SPI_Tx_buffer[5] = NULL_DATA;
	sensor->SPI_Tx_buffer[6] = GET1_OPCODE;
	sensor->SPI_Tx_buffer[7] = calculate_crc(sensor->SPI_Tx_buffer);
}

static void set_Tx_NOP(Magnetometer_HandleTypeDef *sensor) {
	sensor->SPI_Tx_buffer[0] = NULL_DATA;
	sensor->SPI_Tx_buffer[1] = NULL_DATA;
	sensor->SPI_Tx_buffer[2] = NOP_KEY;
	sensor->SPI_Tx_buffer[3] = NOP_KEY;
	sensor->SPI_Tx_buffer[4] = NULL_DATA;
	sensor->SPI_Tx_buffer[5] = NULL_DATA;
	sensor->SPI_Tx_buffer[6] = NOP_OPCODE;
	sensor->SPI_Tx_buffer[7] = calculate_crc(sensor->SPI_Tx_buffer);
}

static void transmit_blocking(Magnetometer_HandleTypeDef *sensor) {
	HAL_GPIO_WritePin(sensor->SS_port, sensor->SS_pin, 0);
	HAL_SPI_Transmit(sensor->hspi, sensor->SPI_Tx_buffer, 8,
	HAL_MAX_DELAY);
	HAL_GPIO_WritePin(sensor->SS_port, sensor->SS_pin, 1);
}

static void reset_roll_counter(Magnetometer_HandleTypeDef *sensor) {
	set_Tx_GET1(sensor, 1);
	transmit_blocking(sensor);
	HAL_Delay(1);
	set_Tx_NOP(sensor);
	transmit_blocking(sensor);
}

static uint8_t getData(Magnetometer_HandleTypeDef *sensor) {
	uint8_t *RxBuffer = sensor->SPI_Rx_buffer;
	/* 		ERROR CHECKING 		*/
	// Check if the transfer is done to avoid race conditions
	if (!sensor->transfer_is_done) {
		return -1;
	}

	// Check the marker if Alpha (0), otherwise ignore
	if ((RxBuffer[6] >> 6) != 0) {
		sensor->err_packets_cnt++;
		return -1; // Not Alpha result
	}

	// Check if bits 2,3,5 if they are 0x00
	if (RxBuffer[2] != 0x00 || RxBuffer[3] != 0x00 || RxBuffer[5] != 0x00) {
		sensor->err_packets_cnt++;
		return -1; // Probably wrong data
	}

	// Virtual Gain at 0?
	if (RxBuffer[4] == 0x00) {
		sensor->err_packets_cnt++;
		return -1; // Probably wrong data
	}

	// Extract the rolling counter
	uint8_t rollcnt = RxBuffer[6] & 0x3F;
	// calculate the difference between the internal sensor's roll counter and the roll counter
	uint8_t last_rollcnt = sensor->roll_cnt;
	uint8_t diff = (rollcnt - last_rollcnt + 64) % 64;
	if (diff > 1) {
		sensor->err_packets_cnt += diff - 1; // We expect diff=1 usually (no lost packets)
	}
	sensor->roll_cnt = rollcnt;

	// Check the checksum of the message
	if (RxBuffer[7] != calculate_crc(RxBuffer)) {
		sensor->err_packets_cnt++;
		return -1; // CRC not valid
	}

	/* 		EXTRACTING THE DATA 		*/
	// Extract and convert the angle to degrees
	sensor->alpha = (((RxBuffer[1] & 0x3F) << 8) + RxBuffer[0])<<2;
	// Extract the error bits
	sensor->diagnostic_bits = RxBuffer[1] >> 6;
	return 0;
}
