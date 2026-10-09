/*
 * watchdog.c
 *
 *  Created on: 7 oct. 2026
 *      Author: raffi
 *
 *  The interface and the two phases of the watchdog are in watchdog.h
 */

#include "watchdog.h"
#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

/* 		SETTINGS 		*/

// timeout of the startup, the same one the bootloader starts the watchdog with
#define WATCHDOG_STARTUP_TIMEOUT_MS 20000u

// Timeout of the supervision. The PWM of the motor is made by a timer : a
// firmware that is stuck keeps pushing with the last force, so keep it short.
// The clock of the watchdog (LSI) isn't precise, it runs between 30 and 60 kHz :
// the real timeout is between 2/3 and 3/2 of this value
#define WATCHDOG_SUPERVISION_TIMEOUT_MS 500u

// time between two checks of the sources during the supervision
#define WATCHDOG_CHECK_PERIOD_MS 100u

// the sources that have to check in between two checks
#define WATCHDOG_REQUIRED_SOURCES (WATCHDOG_SOURCE_CONTROL_LOOP \
		| WATCHDOG_SOURCE_SYSTICK | WATCHDOG_SOURCE_SENSOR)

// Even with the shortest real timeout (2/3 of the value), one check has to be
// able to fail without a reset : the next one must still arrive in time
#if (WATCHDOG_SUPERVISION_TIMEOUT_MS * 2u / 3u) < (2u * WATCHDOG_CHECK_PERIOD_MS)
#error "WATCHDOG_SUPERVISION_TIMEOUT_MS is too short for WATCHDOG_CHECK_PERIOD_MS"
#endif

/* 		IWDG REGISTERS 		*/

// typical frequency of the LSI
#define WATCHDOG_LSI_FREQ_HZ 40000u
// values to write in the key register (IWDG->KR)
#define WATCHDOG_KEY_FEED 0xAAAAu
#define WATCHDOG_KEY_UNLOCK 0x5555u
#define WATCHDOG_KEY_START 0xCCCCu
// the divider of the LSI is 4 << prescaler, with a prescaler from 0 to 6
#define WATCHDOG_FIRST_DIVIDER 4u
#define WATCHDOG_MAX_PRESCALER 6u
// the counter is 12 bits wide
#define WATCHDOG_MAX_COUNT 0x1000u
// longest time given to the watchdog to take a new timeout into account
#define WATCHDOG_REGISTER_UPDATE_TIMEOUT_MS 100u

// One bit per source that checked in since the last check.
// Written by every context (watchdog_checkin) : it is only read and written
// with atomic operations, so no check-in can be lost
static volatile uint32_t checked_in_sources;
// time of the last check (HAL_GetTick), main thread only
static uint32_t last_check_time;
// false during the startup, true during the supervision. main thread only
static bool supervising;

static void feed(void) {
  // does nothing if the watchdog was never started
  IWDG->KR = WATCHDOG_KEY_FEED;
}

// number of LSI periods, after the divider, that fit in timeout_ms
static uint32_t count_for(uint32_t timeout_ms, uint32_t divider) {
  return (timeout_ms * WATCHDOG_LSI_FREQ_HZ) / (divider * 1000u);
}

// Sets the timeout of the watchdog, then feeds it.
// It also starts the watchdog if nobody did it yet
static void set_timeout(uint32_t timeout_ms) {
  // take the smallest divider whose count fits in the counter : it is the
  // one that gives the most precise timeout
  uint32_t prescaler = 0u;
  uint32_t divider = WATCHDOG_FIRST_DIVIDER;
  uint32_t count = count_for(timeout_ms, divider);
  while (count > WATCHDOG_MAX_COUNT && prescaler < WATCHDOG_MAX_PRESCALER) {
    prescaler++;
    divider <<= 1;
    count = count_for(timeout_ms, divider);
  }
  if (count > WATCHDOG_MAX_COUNT) {
    count = WATCHDOG_MAX_COUNT;
  }
  if (count == 0u) {
    count = 1u;
  }

  // starting a watchdog that already runs does nothing
  IWDG->KR = WATCHDOG_KEY_START;
  // the two registers below can't be written without this
  IWDG->KR = WATCHDOG_KEY_UNLOCK;
  IWDG->PR = prescaler;
  IWDG->RLR = count - 1u;

  // The watchdog needs a few LSI periods to take the new values. If it takes
  // too long, don't stay stuck here : it is fed anyway and the new values
  // apply when they are ready
  uint32_t start = HAL_GetTick();
  while (IWDG->SR != 0u
      && (HAL_GetTick() - start) <= WATCHDOG_REGISTER_UPDATE_TIMEOUT_MS) {}

  feed();
}

void watchdog_init(void) {
  supervising = false;
  set_timeout(WATCHDOG_STARTUP_TIMEOUT_MS);
}

void watchdog_start_supervision(void) {
  set_timeout(WATCHDOG_SUPERVISION_TIMEOUT_MS);
  last_check_time = HAL_GetTick();
  // forget what checked in during the startup
  (void)__atomic_exchange_n(&checked_in_sources, 0u, __ATOMIC_RELAXED);
  supervising = true;
}

void watchdog_checkin(watchdog_source_t source) {
  __atomic_fetch_or(&checked_in_sources, (uint32_t)source, __ATOMIC_RELAXED);
}

void watchdog_task(void) {
  // startup : waiting is normal, feed every time
  if (!supervising) {
    feed();
    return;
  }

  uint32_t current_time = HAL_GetTick();
  if (current_time - last_check_time < WATCHDOG_CHECK_PERIOD_MS) {
    return;
  }
  last_check_time = current_time;

  // read the sources and clear them for the next check in one step
  uint32_t sources = __atomic_exchange_n(&checked_in_sources, 0u,
  __ATOMIC_RELAXED);
  if ((sources & WATCHDOG_REQUIRED_SOURCES) == WATCHDOG_REQUIRED_SOURCES) {
    feed();
  }
  // otherwise the watchdog isn't fed : if a source stays silent until the
  // timeout, the board resets and the bootloader takes over
}
