/*
 * wheel_file_error.h
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Content of a WHEEL_FILE_ERROR file (see wheel_file.h), and the error codes
 *  it carries. The host decodes both
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_ERROR_H_
#define FILE_SYSTEM_WHEEL_FILE_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// Version of wheel_file_error_t, for the header of the file. Changes with the
// layout of the struct
#define WHEEL_FILE_ERROR_VERSION 1

// What went wrong. The values are part of the file: a code keeps its value, a
// new one goes at the end of the list
typedef enum {
  WHEEL_ERR_NONE = 0, // never sent
  // Faults of the processor: fatal (wheel_fatal.c)
  WHEEL_ERR_NMI = 1,
  WHEEL_ERR_HARD_FAULT = 2,
  WHEEL_ERR_MEM_MANAGE = 3,
  WHEEL_ERR_BUS_FAULT = 4,
  WHEEL_ERR_USAGE_FAULT = 5,
  // Fatal, found by the firmware
  WHEEL_ERR_HAL = 6, // Error_Handler(): an init of main.c or a HAL call failed
  WHEEL_ERR_FFB = 7, // the ffb library returned no object
  // Startup
  WHEEL_ERR_INIT = 8, // a module did not initialize or start
  WHEEL_ERR_CALIBRATION = 9, // CALIBRATION_MAX_TRIES failed, then each retry
  // Frames of the magnetometer
  WHEEL_ERR_SENSOR_MARKER = 10, // the answer is not an Alpha result
  WHEEL_ERR_SENSOR_DATA = 11, // bytes 2, 3 or 5 not 0, or a virtual gain of 0
  WHEEL_ERR_SENSOR_CRC = 12,
  WHEEL_ERR_SENSOR_MISSED = 13, // the rolling counter skipped: frames lost
  WHEEL_ERR_SENSOR_SPI = 14, // a transfer refused by HAL, or an SPI/DMA error
  // Others
  WHEEL_ERR_ANALOG = 15, // a DMA error of the ADC
  WHEEL_ERR_MOTOR = 16, // Apply_Force() refused the force
  WHEEL_ERR_COUNT // not a code: the number of them
} wheel_error_t;

// Size of file in wheel_file_error_t, its terminating zero included
#define WHEEL_ERROR_FILE_NAME_SIZE 24
// Return addresses kept in trace
#define WHEEL_ERROR_TRACE_DEPTH 16

// What happened and where. Also the start of a WHEEL_FILE_DUMP file
typedef struct {
  uint16_t code; // wheel_error_t
  uint16_t line; // of the call in file, 0 for a fault of the processor
  // Exception number (IPSR) of the code that failed: 0 in main, 15 in
  // SysTick, 16 + IRQn in an interrupt
  uint16_t context;
  uint16_t count; // occurrences merged in this record, stops at 0xFFFF
  uint32_t time_ms; // HAL_GetTick() of the first one: ms since the start
  // Source file of the call, without its folders. Ends with a zero: a longer
  // name is cut. Empty for a fault of the processor
  char file[WHEEL_ERROR_FILE_NAME_SIZE];
  // Return addresses found on the stack at that moment, the most recent
  // first, 0 after the last one. Each one follows the call that led there:
  // addr2line on (address - 1) gives the line of that call
  uint32_t trace[WHEEL_ERROR_TRACE_DEPTH];
} wheel_file_error_t;

// The host reads the fields at these places
static_assert(offsetof(wheel_file_error_t, time_ms) == 8, "a field moved");
static_assert(offsetof(wheel_file_error_t, file) == 12, "a field moved");
static_assert(offsetof(wheel_file_error_t, trace) == 36, "a field moved");
static_assert(sizeof(wheel_file_error_t) == 100, "not 100 bytes");

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_ERROR_H_ */
