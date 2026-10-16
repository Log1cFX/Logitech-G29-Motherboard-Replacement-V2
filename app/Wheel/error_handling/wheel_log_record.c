/*
 * wheel_log_record.c
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  What and where of an error (wheel_file_error_t), for wheel_log.c and
 *  wheel_fatal.c: the record, its trace, and their printing in DEBUG builds.
 *
 *  TRACE
 *  There are no frame pointers to follow: the stack is scanned instead, from
 *  where the error happened up to its top, for the return addresses that
 *  calls left there. A word is taken for one when:
 *    - its bit 0 is set (Thumb code)
 *    - it points inside the code of the firmware
 *    - the instruction right before it is a call: BL, or BLX with a register
 *  A word that passes by chance (an old return address left in a variable)
 *  makes an extra entry: the trace is a strong hint, not a proof
 */

#include "wheel_log_internal.h"
#include "main.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Code of the firmware, from the vector table to the end of .text, and the top
// of the stack (linker script)
extern uint32_t _sapp[];
extern uint32_t _etext[];
extern uint32_t _estack[];

// The two half-words of a BL, and a BLX with a register: ARMv7-M Architecture
// Reference Manual, encodings T1
#define BL_FIRST_MASK 0xF800u
#define BL_FIRST 0xF000u
#define BL_SECOND_MASK 0xD000u
#define BL_SECOND 0xD000u
#define BLX_REGISTER_MASK 0xFF87u
#define BLX_REGISTER 0x4780u

// address, a return address without its Thumb bit, comes right after a call:
// a BL ends there (4 bytes), or a BLX with a register (2 bytes)
static bool follows_call(uint32_t address) {
  const uint16_t *after = (const uint16_t*)address;
  uint16_t bl_first = after[-2];
  uint16_t bl_second = after[-1];
  if ((bl_first & BL_FIRST_MASK) == BL_FIRST && (bl_second & BL_SECOND_MASK)
      == BL_SECOND) {
    return true;
  }
  return (after[-1] & BLX_REGISTER_MASK) == BLX_REGISTER;
}

// value is a return address into the code from code_start to code_end
static bool is_return_address(uint32_t value, uint32_t code_start, uint32_t code_end) {
  if ((value & 1u) == 0u) {
    return false;
  }
  uint32_t address = value & ~1u;
  // A call of 4 bytes before it must still be inside the code
  if (address < code_start + 4u || address > code_end) {
    return false;
  }
  return follows_call(address);
}

// See TRACE above. The entries are kept without their Thumb bit
uint8_t wheel_log_trace_scan(uint32_t *trace, uint32_t first, const uint32_t *sp,
                             const uint32_t *top, uint32_t code_start,
                             uint32_t code_end) {
  uint8_t count = 0;
  if (is_return_address(first, code_start, code_end)) {
    trace[count++] = first & ~1u;
  }

  // A stack pointer outside the RAM, or not on a word, would make the scan
  // read anything: the trace keeps first only
  bool valid_sp = (uintptr_t)sp >= SRAM_BASE && sp < top
                  && ((uintptr_t)sp & 3u) == 0u;
  for (const uint32_t *word = sp;
      valid_sp && word < top && count < WHEEL_ERROR_TRACE_DEPTH; word++) {
    uint32_t value = *word;
    if (!is_return_address(value, code_start, code_end)) {
      continue;
    }
    // The same address twice in a row: the copy that a function saves of its
    // own return address, right after first
    if (count > 0u && trace[count - 1u] == (value & ~1u)) {
      continue;
    }
    trace[count++] = value & ~1u;
  }

  // 0 after the last one
  for (uint8_t i = count; i < WHEEL_ERROR_TRACE_DEPTH; i++) {
    trace[i] = 0u;
  }
  return count;
}

// The fields in their order, then the trace. The name is cut to leave room for
// its terminating zero
void wheel_log_record_fill(wheel_file_error_t *record, wheel_error_t code,
                           const char *file, uint16_t line, uint16_t context,
                           uint32_t first, const uint32_t *sp) {
  // The zeros after the name come from here
  memset(record, 0, sizeof(*record));
  record->code = (uint16_t)code;
  record->line = line;
  record->context = context;
  record->count = 1u;
  record->time_ms = HAL_GetTick();
  if (file != NULL) {
    strncpy(record->file, file, sizeof(record->file) - 1u);
  }
  wheel_log_trace_scan(record->trace, first, sp, _estack, (uint32_t)(uintptr_t)_sapp,
                       (uint32_t)(uintptr_t)_etext);
}

#ifdef DEBUG

// Names of the codes for the console, NULL for a code without one
static const char *const code_names[WHEEL_ERR_COUNT] = {
  [WHEEL_ERR_NMI] = "NMI",
  [WHEEL_ERR_HARD_FAULT] = "HARD_FAULT",
  [WHEEL_ERR_MEM_MANAGE] = "MEM_MANAGE",
  [WHEEL_ERR_BUS_FAULT] = "BUS_FAULT",
  [WHEEL_ERR_USAGE_FAULT] = "USAGE_FAULT",
  [WHEEL_ERR_HAL] = "HAL",
  [WHEEL_ERR_FFB] = "FFB",
  [WHEEL_ERR_INIT] = "INIT",
  [WHEEL_ERR_CALIBRATION] = "CALIBRATION",
  [WHEEL_ERR_SENSOR_MARKER] = "SENSOR_MARKER",
  [WHEEL_ERR_SENSOR_DATA] = "SENSOR_DATA",
  [WHEEL_ERR_SENSOR_CRC] = "SENSOR_CRC",
  [WHEEL_ERR_SENSOR_MISSED] = "SENSOR_MISSED",
  [WHEEL_ERR_SENSOR_SPI] = "SENSOR_SPI",
  [WHEEL_ERR_ANALOG] = "ANALOG",
  [WHEEL_ERR_MOTOR] = "MOTOR"
};

// One line of what and where, then one of trace
void wheel_log_print_error(const wheel_file_error_t *record) {
  const char *name = NULL;
  if (record->code < WHEEL_ERR_COUNT) {
    name = code_names[record->code];
  }
  printf("[%" PRIu32 " ms] error %u %s at %s:%u, context %u, x%u\n",
         record->time_ms, record->code, (name != NULL) ? name : "?",
         record->file, record->line, record->context, record->count);
  printf("  trace");
  for (uint8_t i = 0; i < WHEEL_ERROR_TRACE_DEPTH && record->trace[i] != 0u; i++) {
    printf(" %08" PRIx32, record->trace[i]);
  }
  printf("\n");
}

// The record, then the registers, as wheel_file_dump_t orders them
void wheel_log_print_dump(const wheel_file_dump_t *dump) {
  printf("FATAL ");
  wheel_log_print_error(&dump->error);
  for (uint8_t i = 0; i < 13u; i++) {
    printf("  r%-2u %08" PRIx32 "%s", i, dump->r[i], (i % 4u == 3u) ? "\n" : "");
  }
  printf("\n  sp  %08" PRIx32 "  lr  %08" PRIx32 "  pc  %08" PRIx32
         "  xpsr %08" PRIx32 "\n", dump->sp, dump->lr, dump->pc, dump->xpsr);
  printf("  exc_return %08" PRIx32 "  primask %" PRIu32 "  basepri %" PRIu32
         "  faultmask %" PRIu32 "  control %" PRIu32 "\n", dump->exc_return,
         dump->primask, dump->basepri, dump->faultmask, dump->control);
  printf("  cfsr %08" PRIx32 "  hfsr %08" PRIx32 "  mmfar %08" PRIx32
         "  bfar %08" PRIx32 "  shcsr %08" PRIx32 "\n", dump->cfsr, dump->hfsr,
         dump->mmfar, dump->bfar, dump->shcsr);
}

#endif /* DEBUG */
