/*
 * wheel_log_internal.h
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Shared by the files of the log module only: wheel_log.c, wheel_fatal.c and
 *  wheel_log_record.c. The interface of the module is wheel_log.h
 */

#ifndef ERROR_HANDLING_WHEEL_LOG_INTERNAL_H_
#define ERROR_HANDLING_WHEEL_LOG_INTERNAL_H_

#include "wheel_log.h"
#include "wheel_file_dump.h"

/* FROM wheel_log_record.c */

// Fills record: code, place, context (IPSR), the time, a count of 1 and the
// trace. file can be NULL (a fault: no place). The trace starts with first
// when it is a return address, then scans the stack from sp up to its top
void wheel_log_record_fill(wheel_file_error_t *record, wheel_error_t code,
                           const char *file, uint16_t line, uint16_t context,
                           uint32_t first, const uint32_t *sp);

// The scan of wheel_log_record_fill(), with the bounds as parameters: the
// stack from sp to top, the code from code_start to code_end. Fills
// trace[WHEEL_ERROR_TRACE_DEPTH], 0 after the last address, and returns how
// many it found
uint8_t wheel_log_trace_scan(uint32_t *trace, uint32_t first, const uint32_t *sp,
                             const uint32_t *top, uint32_t code_start,
                             uint32_t code_end);

#ifdef DEBUG
// One record, then one dump, on the debug console (printf)
void wheel_log_print_error(const wheel_file_error_t *record);
void wheel_log_print_dump(const wheel_file_dump_t *dump);
#endif

#endif /* ERROR_HANDLING_WHEEL_LOG_INTERNAL_H_ */
