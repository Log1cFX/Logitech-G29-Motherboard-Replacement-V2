/*
 * wheel_file_dump.h
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Content of a WHEEL_FILE_DUMP file (see wheel_file.h): the state of the
 *  processor when a fatal error stopped the firmware. Written to its flash page
 *  (wheel_file_pages.h) before the reset, sent after it
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_DUMP_H_
#define FILE_SYSTEM_WHEEL_FILE_DUMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_file_error.h"

// Version of wheel_file_dump_t, for the header of the file. Changes with the
// layout of the struct
#define WHEEL_FILE_DUMP_VERSION 1

typedef struct {
  // What and where. For a fault of the processor: line 0, file empty, and the
  // place is pc
  wheel_file_error_t error;
  // Core registers at the fault, or at the call of WHEEL_FATAL_ERROR(). There,
  // sp is the one of the caller and pc the return address of the call
  uint32_t r[13]; // r0 to r12
  uint32_t sp;
  uint32_t lr;
  uint32_t pc;
  uint32_t xpsr;
  uint32_t exc_return; // lr of the fault handler, 0 for WHEEL_FATAL_ERROR()
  uint32_t primask;
  uint32_t basepri;
  uint32_t faultmask;
  uint32_t control;
  // Fault status registers of the System Control Block
  uint32_t cfsr;
  uint32_t hfsr;
  uint32_t mmfar;
  uint32_t bfar;
  uint32_t shcsr;
} wheel_file_dump_t;

// The host reads the fields at these places
static_assert(offsetof(wheel_file_dump_t, r) == 100, "a field moved");
static_assert(offsetof(wheel_file_dump_t, exc_return) == 168, "a field moved");
static_assert(offsetof(wheel_file_dump_t, cfsr) == 188, "a field moved");
static_assert(sizeof(wheel_file_dump_t) == 208, "not 208 bytes");

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_DUMP_H_ */
