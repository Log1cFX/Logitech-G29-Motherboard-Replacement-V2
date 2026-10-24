/*
 * wheel_log.h
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Errors and logs of the firmware. Each one leaves as a file (wheel_file.h):
 *
 *    WHEEL_LOG()          text            WHEEL_FILE_LOG     from level 2
 *    WHEEL_LOG_ERROR()    soft error      WHEEL_FILE_ERROR   from level 1
 *    WHEEL_FATAL_ERROR()  fatal error     WHEEL_FILE_DUMP    from level 1
 *    fault of the processor               WHEEL_FILE_DUMP    from level 1
 *
 *  DEBUG builds print them through semihosting, the others send them to the
 *  application of the PC over the file link (usb_msg.h).
 *
 *  A soft error is recorded where it happens, in any context, then sent by
 *  wheel_log_task(): see wheel_log.c. The wheel goes on.
 *  A fatal error stops the motor, writes the dump to its flash page and resets,
 *  at every level. After the restart, wheel_log_task() sends the dump, then
 *  clears its page: see wheel_fatal.c. DEBUG builds halt in the debugger
 *  instead.
 */

#ifndef ERROR_HANDLING_WHEEL_LOG_H_
#define ERROR_HANDLING_WHEEL_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wheel_file_error.h"
#include "wheel_file_settings.h"
#include <stdint.h>

/* SETTINGS */
// 0: nothing is logged, the errors are only counted. 1: errors and dumps.
// 2: also the text of WHEEL_LOG(). 1 on a wheel in use.
// The ceiling of the build: what is above it is not even compiled. The level
// in use is the one of the settings (log_level), which cannot exceed it
#define WHEEL_LOG_LEVEL 2

/* USE */
// The source file of a call, without its folders (GCC 12 and later)
#ifdef __FILE_NAME__
#define WHEEL_LOG_FILE __FILE_NAME__
#else
#define WHEEL_LOG_FILE __FILE__
#endif

// Text, like printf. Main only: dropped in an interrupt. Below level 2 the call
// is removed, its arguments are not evaluated
#if WHEEL_LOG_LEVEL >= 2
#define WHEEL_LOG(...) wheel_log(__VA_ARGS__)
#else
#define WHEEL_LOG(...) ((void)0)
#endif

// An error the wheel goes on after, code a wheel_error_t. Any context
#define WHEEL_LOG_ERROR(code) wheel_log_error((code), WHEEL_LOG_FILE, __LINE__)

// An error the wheel cannot go on after. Any context, never returns
#define WHEEL_FATAL_ERROR(code) wheel_fatal_error((code), WHEEL_LOG_FILE, __LINE__)

// Takes the dump left by the last run, and gives their own handlers to the
// configurable faults of the processor. First call of wheel_startup()
void wheel_log_init(void);

// The settings that hold the level in use, in the settings image
// (wheel_settings.h), read at each log. Until this call, WHEEL_LOG_LEVEL is
// used. Main, once the settings are loaded
void wheel_log_set_settings(const wheel_file_settings_system_t *settings);

// Sends the dump of the last run, then the errors recorded since. Main only,
// from every usb_task() (see usb_custom_task() in steeringwheel.c)
void wheel_log_task(void);

// The soft errors since the start by code, WHEEL_ERR_COUNT values in counts.
// Then the errors that could not be recorded, and the texts of WHEEL_LOG()
// that could not be sent. For the device info (wheel_command.c). Main
void wheel_log_counts(uint32_t counts[WHEEL_ERR_COUNT], uint32_t *dropped_errors,
                      uint32_t *dropped_logs);

/* BEHIND THE MACROS */
// In wheel_log.c
void wheel_log(const char *format, ...) __attribute__((format(printf, 1, 2)));
void wheel_log_error(wheel_error_t code, const char *file, uint16_t line);
// In wheel_fatal.c
void wheel_fatal_error(wheel_error_t code, const char *file, uint16_t line)
    __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLING_WHEEL_LOG_H_ */
