/*
 * watchdog.c
 *
 *  Created on: 7 oct. 2026
 *      Author: raffi
 *
 *  See watchdog.h for the interface and the two phases
 */

#include "watchdog.h"
#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

/* SETTINGS */

// The timeout the bootloader starts the watchdog with
#define WATCHDOG_STARTUP_TIMEOUT_MS 20000u

// Keep it short: the PWM of the motor comes from a timer, so a stuck firmware
// keeps pushing with the last force. The clock of the watchdog (LSI) runs
// between 30 and 60 kHz: the real timeout is 2/3 to 3/2 of this value
#define WATCHDOG_SUPERVISION_TIMEOUT_MS 500u

// Period of the source checks during the supervision
#define WATCHDOG_CHECK_PERIOD_MS 100u

#define WATCHDOG_REQUIRED_SOURCES (WATCHDOG_SOURCE_CONTROL_LOOP \
		| WATCHDOG_SOURCE_SYSTICK | WATCHDOG_SOURCE_SENSOR)

// One failed check must not reset the board: even with the shortest real
// timeout (2/3), the next check still has to arrive in time
#if (WATCHDOG_SUPERVISION_TIMEOUT_MS * 2u / 3u) < (2u * WATCHDOG_CHECK_PERIOD_MS)
#error "WATCHDOG_SUPERVISION_TIMEOUT_MS is too short for WATCHDOG_CHECK_PERIOD_MS"
#endif

/* IWDG REGISTERS */

#define WATCHDOG_LSI_FREQ_HZ 40000u // typical
// Keys of IWDG->KR
#define WATCHDOG_KEY_FEED 0xAAAAu
#define WATCHDOG_KEY_UNLOCK 0x5555u // allows writing PR and RLR
#define WATCHDOG_KEY_START 0xCCCCu
// The LSI is divided by 4 << prescaler, with a prescaler from 0 to 6
#define WATCHDOG_FIRST_DIVIDER 4u
#define WATCHDOG_MAX_PRESCALER 6u
#define WATCHDOG_MAX_COUNT 0x1000u // the counter is 12 bits wide
// Longest wait for the watchdog to take a new prescaler and reload value
#define WATCHDOG_REGISTER_UPDATE_TIMEOUT_MS 100u

// One bit per source that checked in since the last check. Written from every
// context, only through atomic operations: no check-in can be lost
static volatile uint32_t checked_in_sources;
static uint32_t last_check_time; // HAL_GetTick() of the last check, main only
static bool supervising; // false during the startup, main only

static void feed(void) {
  // No effect if the watchdog was never started
  IWDG->KR = WATCHDOG_KEY_FEED;
}

// LSI periods, after the divider, that fit in timeout_ms
static uint32_t count_for(uint32_t timeout_ms, uint32_t divider) {
  return (timeout_ms * WATCHDOG_LSI_FREQ_HZ) / (divider * 1000u);
}

// Sets the timeout and feeds. Starts the watchdog if it is not running yet
static void set_timeout(uint32_t timeout_ms) {
  // The smallest divider whose count fits in the counter is the most precise
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

  IWDG->KR = WATCHDOG_KEY_START; // no effect if it already runs
  IWDG->KR = WATCHDOG_KEY_UNLOCK;
  IWDG->PR = prescaler;
  IWDG->RLR = count - 1u;

  // The new values take a few LSI periods to apply. Do not wait forever: the
  // watchdog is fed anyway and they apply when they are ready
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
  // Drop the check-ins of the startup
  (void)__atomic_exchange_n(&checked_in_sources, 0u, __ATOMIC_RELAXED);
  supervising = true;
}

void watchdog_checkin(watchdog_source_t source) {
  __atomic_fetch_or(&checked_in_sources, (uint32_t)source, __ATOMIC_RELAXED);
}

void watchdog_task(void) {
  if (!supervising) {
    feed();
    return;
  }

  uint32_t current_time = HAL_GetTick();
  if (current_time - last_check_time < WATCHDOG_CHECK_PERIOD_MS) {
    return;
  }
  last_check_time = current_time;

  // Read the sources and clear them for the next check, in one step
  uint32_t sources = __atomic_exchange_n(&checked_in_sources, 0u,
  __ATOMIC_RELAXED);
  if ((sources & WATCHDOG_REQUIRED_SOURCES) == WATCHDOG_REQUIRED_SOURCES) {
    feed();
  }
  // Not fed otherwise: if a source stays silent until the timeout, the board
  // resets and the bootloader takes over
}
