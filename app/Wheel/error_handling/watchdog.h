/*
 * watchdog.h
 *
 *  Created on: 7 oct. 2026
 *      Author: raffi
 *
 *  The watchdog of the wheel : the independent watchdog of the STM32 (IWDG).
 *
 *  The bootloader starts it right before it jumps to the firmware, with a long
 *  timeout (about 20 s). A watchdog that is started can't be stopped anymore :
 *  it has to be fed before the timeout, otherwise it resets the board. After
 *  such a reset, the bootloader stays in DFU mode instead of starting the
 *  firmware again.
 *
 *  The firmware uses it in two phases :
 *
 *    startup      long timeout, fed in every watchdog_task(). Waiting is
 *                 normal there (the host that enumerates, the calibration)
 *    supervision  short timeout, fed only if every source of
 *                 watchdog_source_t checked in since the last check. If one of
 *                 them stops, the watchdog isn't fed anymore and the board
 *                 resets
 *
 *  watchdog_task() is the only function that feeds the watchdog, and it has to
 *  be called from the main thread only : an interrupt keeps running when the
 *  main thread is stuck, feeding from there would hide the problem.
 *
 *  The module doesn't know about DEBUG. It is the wheel that leaves the
 *  watchdog alone in DEBUG, by not calling anything here : see wheel_startup()
 */

#ifndef ERROR_HANDLING_WATCHDOG_H_
#define ERROR_HANDLING_WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

// The contexts that have to show that they are alive during the supervision.
// To add one : add it here and in WATCHDOG_REQUIRED_SOURCES (watchdog.c)
typedef enum {
	// the main thread did one pass of the control loop
	WATCHDOG_SOURCE_CONTROL_LOOP = (1u << 0),
	// SysTick ran : the controls were read
	WATCHDOG_SOURCE_SYSTICK = (1u << 1),
	// the magnetometer answered with a valid frame
	WATCHDOG_SOURCE_SENSOR = (1u << 2)
} watchdog_source_t;

// Takes over the watchdog started by the bootloader, with the long timeout of
// the startup. If the bootloader didn't start it, it is started here.
// To call once, before anything that can take time
void watchdog_init(void);

// End of the startup : sets the short timeout and begins the supervision.
// To call right before the control loop starts
void watchdog_start_supervision(void);

// Tells the watchdog that a source is alive. Can be called from any context,
// the interrupts included
void watchdog_checkin(watchdog_source_t source);

// Feeds the watchdog when it has to be fed (see the two phases above).
// To call from the main thread, as often as possible, and also inside every
// wait of the main thread
void watchdog_task(void);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLING_WATCHDOG_H_ */
