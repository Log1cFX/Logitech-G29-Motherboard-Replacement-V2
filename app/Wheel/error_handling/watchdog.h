/*
 * watchdog.h
 *
 *  Created on: 7 oct. 2026
 *      Author: raffi
 *
 *  The independent watchdog (IWDG) of the STM32.
 *
 *  The bootloader starts it with a timeout of about 20 s, right before it jumps
 *  to the firmware. Once started it cannot be stopped: if it is not fed in
 *  time the board resets, and the bootloader then stays in DFU mode.
 *
 *  The firmware uses it in two phases:
 *
 *    startup      long timeout, fed by every watchdog_task(). Long waits are
 *                 expected there (enumeration, calibration)
 *    supervision  short timeout, fed only when every source of
 *                 watchdog_source_t checked in since the last check
 *
 *  Only watchdog_task() feeds, and only from main: an interrupt keeps running
 *  when main is stuck, feeding from one would hide it.
 *
 *  DEBUG builds do not use this module, see wheel_startup()
 */

#ifndef ERROR_HANDLING_WATCHDOG_H_
#define ERROR_HANDLING_WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

// What has to prove that it is alive during the supervision.
// A new source also goes in WATCHDOG_REQUIRED_SOURCES (watchdog.c)
typedef enum {
  WATCHDOG_SOURCE_CONTROL_LOOP = (1u << 0), // main ran one control loop step
  WATCHDOG_SOURCE_SYSTICK = (1u << 1), // SysTick read the controls
  WATCHDOG_SOURCE_SENSOR = (1u << 2) // the magnetometer sent a valid frame
} watchdog_source_t;

// Sets the startup timeout, and starts the watchdog if the bootloader did not.
// Call once, before anything slow
void watchdog_init(void);

// Sets the supervision timeout. Call right before the control loop starts
void watchdog_start_supervision(void);

// Marks a source as alive. Can be called from any context, interrupts included
void watchdog_checkin(watchdog_source_t source);

// Feeds when the current phase allows it. Call from main as often as possible,
// including inside every wait
void watchdog_task(void);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLING_WATCHDOG_H_ */
