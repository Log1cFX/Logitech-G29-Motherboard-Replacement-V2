/*
 * wheel_log.c
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Soft errors, text logs, and the dump of the last run: see wheel_log.h.
 *
 *  PENDING ERRORS
 *  A soft error is recorded in the context where it happens, then waits in a
 *  ring, the oldest first, for wheel_log_task(). The same call failing again
 *  meanwhile only adds to the count of its record: an error on every frame of
 *  the sensor costs one record per WHEEL_LOG_MERGE_MS. Records come from every
 *  context: the ring is only changed with the interrupts off.
 *
 *  OUTPUT
 *  DEBUG builds print through semihosting. The others send files over the file
 *  link (usb_msg.h), which keeps them until an application of the PC connects
 */

#include "wheel_log_internal.h"
#include "wheel_file_pages.h"
#include "wheel_file_log.h"
#include "usb_msg.h"
#include "main.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* SETTINGS */
// Time a record waits before it is sent. The same error meanwhile only adds to
// its count
#define WHEEL_LOG_MERGE_MS 1000u
// Records that can wait at once, each from a different call
#define WHEEL_LOG_PENDING 8u

#define NO_SLOT WHEEL_LOG_PENDING

static struct {
  // Every soft error since the start, by code, merged and dropped ones
  // included. Any context: atomic additions
  uint32_t counts[WHEEL_ERR_COUNT];

  // See PENDING ERRORS above. pending_file keeps the file pointer of each
  // call: one call always passes the same one
  wheel_file_error_t pending[WHEEL_LOG_PENDING];
  const char *pending_file[WHEEL_LOG_PENDING];
  uint8_t head; // slot of the oldest record
  uint8_t used; // records waiting

  // The dump of the last run in its page, NULL when there is none to send.
  // Main only
  const wheel_file_t *dump;
  bool dump_queued; // given to usb_msg, which waits for its handshake

  // For the debugger. Any context: atomic additions
  uint32_t dropped_errors; // the ring was full
  uint32_t dropped_text; // usb_msg was full, or WHEEL_LOG() in an interrupt
} logs;

#if WHEEL_LOG_LEVEL >= 1

/* PENDING ERRORS */

// Slot of the waiting record of the same call, NO_SLOT when there is none.
// Interrupts off
static uint8_t find_pending(wheel_error_t code, const char *file, uint16_t line) {
  for (uint8_t i = 0; i < logs.used; i++) {
    uint8_t slot = (logs.head + i) % WHEEL_LOG_PENDING;
    const wheel_file_error_t *record = &logs.pending[slot];
    if (record->code == code && record->line == line
        && logs.pending_file[slot] == file) {
      return slot;
    }
  }
  return NO_SLOT;
}

// One more occurrence in a record, which stops counting at UINT16_MAX
static void count_up(wheel_file_error_t *record) {
  if (record->count < UINT16_MAX) {
    record->count++;
  }
}

// Adds one occurrence to the waiting record of the same call, if there is one
static bool merge(wheel_error_t code, const char *file, uint16_t line) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint8_t slot = find_pending(code, file, line);
  if (slot != NO_SLOT) {
    count_up(&logs.pending[slot]);
  }
  __set_PRIMASK(primask);
  return slot != NO_SLOT;
}

// Puts a new record at the end of the ring. The same call may have been
// recorded by an interrupt since merge(): it is merged then. Dropped when the
// ring is full
static void add(const wheel_file_error_t *record, const char *file) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  uint8_t slot = find_pending(record->code, file, record->line);
  if (slot != NO_SLOT) {
    count_up(&logs.pending[slot]);
  } else if (logs.used < WHEEL_LOG_PENDING) {
    slot = (logs.head + logs.used) % WHEEL_LOG_PENDING;
    logs.pending[slot] = *record;
    logs.pending_file[slot] = file;
    logs.used++;
  } else {
    __atomic_fetch_add(&logs.dropped_errors, 1u, __ATOMIC_RELAXED);
  }
  __set_PRIMASK(primask);
}

/* OUTPUT */

// The dump of the last run, before anything of this one. The page keeps it
// until it is printed, or until the PC acknowledged or refused it: then it is
// cleared, and the next fatal error can write there
static void send_dump(void) {
  const wheel_file_t *dump = logs.dump;
  if (dump == NULL) {
    return;
  }
#ifdef DEBUG
  // The content is on a multiple of 4 bytes: the page is
  wheel_log_print_dump((const wheel_file_dump_t*)dump->content);
#else
  if (!logs.dump_queued) {
    // usb_msg keeps a copy: the page stays as it is until the handshake
    logs.dump_queued = usb_msg_send(dump->header.type, dump->content,
                                    dump->header.size)
                       == USB_MSG_OK;
    return;
  }
  // The copy in usb_msg has the same crc field
  if (usb_msg_pending(dump->header.crc)) {
    return;
  }
#endif
  // Clear the dump after it has been sent
  // A failed clear leaves the dump in its page: sent again at the next start
  (void)wheel_file_page_clear(WHEEL_FILE_PAGE_DUMP);
  logs.dump = NULL;
}

// The oldest waiting record, once its WHEEL_LOG_MERGE_MS are over. One per call
static void send_error(void) {
  // Only main removes records: used can only grow before the copy below
  if (logs.used == 0u) {
    return;
  }
  // A copy, taken with the interrupts off: an interrupt may add to its count
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  wheel_file_error_t record = logs.pending[logs.head];
  __set_PRIMASK(primask);
  if (HAL_GetTick() - record.time_ms < WHEEL_LOG_MERGE_MS) {
    return;
  }

#ifdef DEBUG
  wheel_log_print_error(&record);
#else
  // No room in usb_msg: the record waits, and keeps counting
  if (usb_msg_send(WHEEL_FILE_ERROR, &record, sizeof(record)) != USB_MSG_OK) {
    return;
  }
#endif

  // What came during the send stays in the ring, as a new record of the same
  // call. Otherwise the record leaves it
  primask = __get_PRIMASK();
  __disable_irq();
  wheel_file_error_t *head = &logs.pending[logs.head];
  head->count -= record.count;
  if (head->count == 0u) {
    logs.head = (logs.head + 1u) % WHEEL_LOG_PENDING;
    logs.used--;
  } else {
    head->time_ms = HAL_GetTick();
  }
  __set_PRIMASK(primask);
}

#endif /* WHEEL_LOG_LEVEL >= 1 */

/* FOR THE WHEEL (wheel_log.h) */

// The dump is only taken if it is a valid one. Anything else that is neither
// erased nor cleared is cleared: the fatal path only writes a page without a
// dump in it (wheel_fatal.c)
void wheel_log_init(void) {
  // MemManage, BusFault and UsageFault get their own handler: otherwise they
  // end as a HardFault, and the dump could not name them
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk
                | SCB_SHCSR_USGFAULTENA_Msk;

  const wheel_file_t *file = wheel_file_page_read(WHEEL_FILE_PAGE_DUMP);
  uint8_t type = wheel_file_page_type(WHEEL_FILE_PAGE_DUMP);
  if (file != NULL && type == WHEEL_FILE_DUMP) {
    logs.dump = file;
  } else if (type != WHEEL_FILE_PAGE_ERASED && type != WHEEL_FILE_PAGE_CLEARED) {
    (void)wheel_file_page_clear(WHEEL_FILE_PAGE_DUMP);
  }
}

// The dump first: it happened before anything of this run. At level 0 it stays
// in its page, for a build that sends it
void wheel_log_task(void) {
#if WHEEL_LOG_LEVEL >= 1
  send_dump();
  send_error();
#endif
}

// Formatted on the stack, then printed (DEBUG) or sent as a WHEEL_FILE_LOG
void wheel_log(const char *format, ...) {
#if WHEEL_LOG_LEVEL >= 2
  // An interrupt could cut into usb_msg, or into a printf of main
  if (__get_IPSR() != 0u) {
    __atomic_fetch_add(&logs.dropped_text, 1u, __ATOMIC_RELAXED);
    return;
  }
  va_list args;
  va_start(args, format);
#ifdef DEBUG
  vprintf(format, args);
#else
  // The file content: its time, then the text and the zero of vsnprintf.
  // Words, so that it is aligned like the struct
  uint32_t words[(sizeof(wheel_file_log_t) + WHEEL_FILE_LOG_MAX_TEXT + 1u + 3u) / 4u];
  wheel_file_log_t *entry = (wheel_file_log_t*)words;
  entry->time_ms = HAL_GetTick();
  int length = vsnprintf(entry->text, WHEEL_FILE_LOG_MAX_TEXT + 1u, format, args);
  if (length >= 0) {
    // vsnprintf returns the length of the whole text: what is cut is gone
    if (length > WHEEL_FILE_LOG_MAX_TEXT) {
      length = WHEEL_FILE_LOG_MAX_TEXT;
    }
    // The zero is not sent
    if (usb_msg_send(WHEEL_FILE_LOG, entry,
                     sizeof(wheel_file_log_t) + (uint16_t)length)
        != USB_MSG_OK) {
      __atomic_fetch_add(&logs.dropped_text, 1u, __ATOMIC_RELAXED);
    }
  }
#endif
  va_end(args);
#else
  (void)format;
#endif
}

// Counted in any case. From level 1: merged into the waiting record of the
// same call, or recorded, with the trace of the place of the call
void wheel_log_error(wheel_error_t code, const char *file, uint16_t line) {
  if ((uint32_t)code < WHEEL_ERR_COUNT) {
    __atomic_fetch_add(&logs.counts[code], 1u, __ATOMIC_RELAXED);
  }
#if WHEEL_LOG_LEVEL >= 1
  // The same call already waits: no new record, no trace
  if (merge(code, file, line)) {
    return;
  }
  // A new one, built with the interrupts on: the scan of the stack takes time.
  // The trace starts with the return address into the caller, then scans
  // the stack above this record: what is below belongs to the calls made from
  // here
  wheel_file_error_t record;
  wheel_log_record_fill(&record, code, file, line, (uint16_t)__get_IPSR(),
                        (uint32_t)(uintptr_t)__builtin_return_address(0),
                        (const uint32_t*)(&record + 1));
  add(&record, file);
#else
  (void)file;
  (void)line;
#endif
}
