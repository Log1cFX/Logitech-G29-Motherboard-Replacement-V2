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
  // Follows the turns of the magnet. Call after every magnetometer transfer
  Wheel_Status (*Update)(struct _Sensor_HandleTypeDef *sensor);
  // Refreshes steering_pos and virtual_axis
  Wheel_Status (*GetAxis)(struct _Sensor_HandleTypeDef *sensor);

  Sensor_ConfigHandleTypeDef Config;

  /*
   * Update() runs in the SPI DMA interrupt (priority 3), GetAxis() in SysTick
   * (priority 6), the calibration and the control loop in main
   * (see the table in wheel_def.h).
   */

  // Written in SysTick (GetAxis), read in main
  volatile int16_t virtual_axis; // steering over the full int16 range, 0 = center
  volatile int32_t steering_pos; // relative to the position of the wheel at startup
  // Ends of the travel, in steering_pos units.
  // Written in main (calibration), read in SysTick (GetAxis)
  volatile int32_t min; // counterclockwise
  volatile int32_t max; // clockwise

  /* PRIVATE */
  uint16_t physical_axis; // SysTick only
  uint16_t previous_sensor_capture; // SPI DMA interrupt only
  // Written in the SPI DMA interrupt (Update), read in SysTick (GetAxis).
  // NOT PROTECTED: the position is turns + capture, but SysTick reads the two
  // one after the other and the SPI DMA interrupt can change both in between
  volatile uint16_t current_sensor_capture;
  volatile int8_t magnet_full_rotation_cnt;
  uint8_t start_settling_cnt; // set by INIT, then SPI DMA interrupt only
  // max - min. Written in SysTick (GetAxis), read in main (calibration)
  volatile uint16_t distance;
  // Written in main (calibration), read in SysTick (GetAxis)
  volatile float axis_scale;
} Sensor_HandleTypeDef;

extern Sensor_HandleTypeDef hSensor;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_SW_SENSOR_H_ */
