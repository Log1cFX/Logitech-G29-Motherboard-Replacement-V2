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
 * sw_sensor.h
 *
 *  Created on: Aug 4, 2025
 *      Author: raffi
 */

#ifndef CORE_DEFINITIONS_SW_SENSOR_H_
#define CORE_DEFINITIONS_SW_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "hw_magnetometer.h"

typedef struct {
  Magnetometer_HandleTypeDef *hw_magnetometer;
} Sensor_ConfigHandleTypeDef;

typedef struct _Sensor_HandleTypeDef {
  Wheel_Status (*INIT)(struct _Sensor_HandleTypeDef *sensor,
                       Sensor_ConfigHandleTypeDef *config);
  Wheel_Status (*DeINIT)(struct _Sensor_HandleTypeDef *sensor);
  Wheel_Status (*Update)(struct _Sensor_HandleTypeDef *sensor);
  Wheel_Status (*GetAxis)(struct _Sensor_HandleTypeDef *sensor);

  // Shouldn't be filled manually but instead by calling INIT
  Sensor_ConfigHandleTypeDef Config;

  /*
   * CONTEXTS (see the table in wheel_def.h)
   * Update() runs in the SPI DMA interrupt (priority 3).
   * GetAxis() runs in SysTick (priority 6).
   * The calibration and the control loop run in the main thread.
   */

  // written by: SysTick (GetAxis) | read by: main thread
  volatile int16_t virtual_axis; // the actual value that's sent, uses full range of int16
  // written by: SysTick (GetAxis) | read by: main thread (calibration)
  volatile int32_t steering_pos; // temporary value relative to wheel's position at startup
  // written by: main thread (calibration) | read by: SysTick (GetAxis)
  volatile int32_t min; // definition of the minimum (counterclockwise) rotation using steering_pos as reference
  volatile int32_t max; // definition of the maximum (clockwise) rotation using steering_pos as reference

  /* THIS IS IMPLEMENTATION SPECIFIC AND ONLY USED INSIDE THE SOURCE FILE */
  uint16_t physical_axis; // SysTick only
  uint16_t previous_sensor_capture; // SPI DMA interrupt only
  // written by: SPI DMA interrupt (Update) | read by: SysTick (GetAxis)
  // These two go together (position = rotations + capture) but they are
  // written and read one after the other. The SPI DMA interrupt can run
  // between the two reads of SysTick, which then gets a pair that doesn't match.
  // NOT PROTECTED YET.
  volatile uint16_t current_sensor_capture;
  volatile int8_t magnet_full_rotation_cnt;
  uint8_t start_settling_cnt; // set by INIT, then SPI DMA interrupt only
  // written by: SysTick (GetAxis) | read by: main thread (calibration)
  volatile uint16_t distance;
  // written by: main thread (calibration) | read by: SysTick (GetAxis)
  volatile float axis_scale;
} Sensor_HandleTypeDef;

// the instance of this module, defined in sw_sensor.c
extern Sensor_HandleTypeDef hSensor;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_SW_SENSOR_H_ */
