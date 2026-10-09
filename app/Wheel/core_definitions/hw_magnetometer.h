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

#define MAGNETOMETER_FRAME_SIZE 8 // bytes of every SPI message, in both directions

typedef struct {
  uint16_t SS_pin;
  GPIO_TypeDef *SS_port;
  SPI_HandleTypeDef *hspi; // used with DMA
  TIM_HandleTypeDef *htim; // one reading per period
} Magnetometer_ConfigHandleTypeDef;

typedef struct _Magnetometer_HandleTypeDef {
  Wheel_Status (*INIT)(struct _Magnetometer_HandleTypeDef *sensor,
                       Magnetometer_ConfigHandleTypeDef *config);
  Wheel_Status (*DeINIT)(struct _Magnetometer_HandleTypeDef *sensor);
  Wheel_Status (*Start_TIM_POLL)(struct _Magnetometer_HandleTypeDef *sensor);
  Wheel_Status (*Stop_TIM_POLL)(struct _Magnetometer_HandleTypeDef *sensor);
  // Call from the period-elapsed callback of htim
  Wheel_Status (*TransmitRecieve_DMA)(struct _Magnetometer_HandleTypeDef *sensor);
  // Call from the transfer-complete callback of hspi.
  // WHEEL_OK: the frame passed every check and reading is updated
  Wheel_Status (*TxRxDone_CB)(struct _Magnetometer_HandleTypeDef *sensor);

  Magnetometer_ConfigHandleTypeDef Config;

  /*
   * Everything below is used by the TIM4 interrupt (TransmitRecieve_DMA) and
   * the SPI DMA interrupt (TxRxDone_CB). Both have priority 3 and cannot
   * interrupt each other, so nothing is volatile (see the table in wheel_def.h).
   * Start_TIM_POLL also uses the buffers from main, before the timer runs.
   */

  // Angle of the magnet over the 16-bit range. Read by the Update() of
  // sw_sensor, in the same SPI DMA interrupt
  uint16_t reading;
  uint16_t err_packets_cnt; // frames missed or rejected

  /* PRIVATE */
  uint8_t SPI_Rx_buffer[MAGNETOMETER_FRAME_SIZE];
  uint8_t SPI_Tx_buffer[MAGNETOMETER_FRAME_SIZE];
  uint8_t transfer_is_done;
  uint8_t diagnostic_bits;
  uint8_t roll_cnt;
} Magnetometer_HandleTypeDef;

extern Magnetometer_HandleTypeDef hmlx90363;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_HW_MAGNETOMETER_H_ */
