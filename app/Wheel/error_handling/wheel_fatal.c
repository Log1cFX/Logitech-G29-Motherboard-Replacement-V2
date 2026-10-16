/*
 * wheel_fatal.c
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Fatal errors: the faults of the processor and WHEEL_FATAL_ERROR(). Both
 *  end here, in this order:
 *
 *    1. interrupts off, motor off
 *    2. the dump: what and where, the registers, the fault status
 *    3. DEBUG builds print it and halt in the debugger: the end
 *    4. the watchdog is fed once, the host sees the wheel unplugged
 *    5. the dump goes to its flash page (wheel_file_pages.h), unless an earlier
 *       one is still there: the first one is kept
 *    6. software reset: the bootloader starts the firmware again, and
 *       wheel_log_task() sends the dump (wheel_log.c)
 *
 *  A crash at every start would loop on 6 without the dump ever being sent. A
 *  fatal error in the first FATAL_LOOP_MS, with the dump of the previous one
 *  still in its page, sets the DFU flag of the bootloader instead (bootloader.h):
 *  the bootloader stays in DFU mode, and a power cycle starts the firmware again.
 *
 *  Nothing here relies on the state of the rest of the firmware: no handle,
 *  no HAL call that waits, a stack of its own.
 *  The CubeMX code generation must not write these handlers (NVIC, "Generate
 *  IRQ handler" unticked for the five of them)
 */

#include "wheel_log_internal.h"
#include "wheel_file_pages.h"
#include "watchdog.h"
#include "bootloader.h"
#include "main.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

// Uptime under which a fatal error, with the dump of the previous one still in
// its page, means a crash at every start
#define FATAL_LOOP_MS 5000u

// How long D+ stays low before the reset when no dump is written: about 10 ms
// at 72 MHz, beyond the 2.5 us the host needs to see an unplug
#define USB_DETACH_LOOPS 100000u

// Its size is also written in the assembly of the entries below
#define FATAL_STACK_SIZE 1024u

// r4 to r11, then for WHEEL_FATAL_ERROR() the sp and the lr of the caller.
// Written by the entries before any C code runs: global for their assembly
__attribute__((used))   uint32_t wheel_fatal_saved[10];

// The stack of the fatal path: the one of the code that failed may be what
// failed. Global for the assembly of the entries
__attribute__((used, aligned(8)))   uint32_t wheel_fatal_stack[FATAL_STACK_SIZE / 4u];
static_assert(sizeof(wheel_fatal_stack) == 1024u,
    "the entries set sp to wheel_fatal_stack + 1024");

// The dump file, built in place: a header, then a wheel_file_dump_t. Words,
// so that it is aligned like a file
static uint32_t dump_file[WHEEL_FILE_TOTAL_SIZE(sizeof(wheel_file_dump_t)) / 4u];

static bool in_fatal_path; // a second entry: a fault inside this path

// The code of a fault is its exception number minus 1 (see wheel_file_error.h).
// The exception number of an IRQn is IRQn + 16
#define FAULT_CODE(irqn) ((irqn) + 16 - 1)
static_assert(WHEEL_ERR_NMI == FAULT_CODE(NonMaskableInt_IRQn), "fault codes");
static_assert(WHEEL_ERR_HARD_FAULT == FAULT_CODE(HardFault_IRQn), "fault codes");
static_assert(WHEEL_ERR_MEM_MANAGE == FAULT_CODE(MemoryManagement_IRQn),
    "fault codes");
static_assert(WHEEL_ERR_BUS_FAULT == FAULT_CODE(BusFault_IRQn), "fault codes");
static_assert(WHEEL_ERR_USAGE_FAULT == FAULT_CODE(UsageFault_IRQn), "fault codes");

void wheel_fatal_from_fault(const uint32_t *frame, uint32_t exc_return) __attribute__((used, noreturn));
void wheel_fatal_from_call(wheel_error_t code, const char *file, uint16_t line) __attribute__((used, noreturn));

/* ENTRIES */

// Shared by the five fault handlers below. Saves r4-r11, which the C code
// could change, takes the exception frame on the stack that was in use (bit 2
// of EXC_RETURN), moves to the fatal stack, then goes on in C
__attribute__((naked)) static void fault_entry(void) {
  __asm volatile(
      "movw r0, #:lower16:wheel_fatal_saved          \n"
      "movt r0, #:upper16:wheel_fatal_saved          \n"
      "stmia r0, {r4-r11}                            \n"
      "tst lr, #4                                    \n"
      "ite eq                                        \n"
      "mrseq r0, msp                                 \n"
      "mrsne r0, psp                                 \n"
      "mov r1, lr                                    \n"
      "movw r2, #:lower16:wheel_fatal_stack + 1024   \n"
      "movt r2, #:upper16:wheel_fatal_stack + 1024   \n"
      "mov sp, r2                                    \n"
      "b wheel_fatal_from_fault                      \n");
}

// The handlers of the vector table (startup_stm32f103cbtx.s)
void NMI_Handler(void) __attribute__((alias("fault_entry")));
void HardFault_Handler(void) __attribute__((alias("fault_entry")));
void MemManage_Handler(void) __attribute__((alias("fault_entry")));
void BusFault_Handler(void) __attribute__((alias("fault_entry")));
void UsageFault_Handler(void) __attribute__((alias("fault_entry")));

// Saves r4-r11 and the sp and lr of the caller, moves to the fatal stack, then
// goes on in C. code, file and line stay in r0 to r2: the C code reads them
__attribute__((naked, noreturn)) void wheel_fatal_error(
    __attribute__((unused))   wheel_error_t code,
    __attribute__((unused)) const char *file, __attribute__((unused))   uint16_t line) {
  __asm volatile(
      "movw r3, #:lower16:wheel_fatal_saved          \n"
      "movt r3, #:upper16:wheel_fatal_saved          \n"
      "mov r12, sp                                   \n"
      "stmia r3, {r4-r12, lr}                        \n"
      "movw r3, #:lower16:wheel_fatal_stack + 1024   \n"
      "movt r3, #:upper16:wheel_fatal_stack + 1024   \n"
      "mov sp, r3                                    \n"
      "b wheel_fatal_from_call                       \n");
}

/* STEPS */

// No force and the drivers off, through the registers: the handles may be
// what failed (channels and pins: init_motor_driver() in wheel_init.h)
static void motor_off(void) {
  TIM1->CCR1 = 0u;
  TIM1->CCR2 = 0u;
  PWM_R_EN_GPIO_Port->BRR = PWM_R_EN_Pin;
  PWM_L_EN_GPIO_Port->BRR = PWM_L_EN_Pin;
}

// First steps of both entries. The interrupt mask is read before the
// interrupts go off, for the dump
static wheel_file_dump_t* enter(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  // A fault inside this path: no second dump, the reset at once
  if (in_fatal_path) {
    NVIC_SystemReset();
  }
  in_fatal_path = true;
  motor_off();

  wheel_file_dump_t *dump = (wheel_file_dump_t*)((wheel_file_t*)dump_file)->content;
  memset(dump, 0, sizeof(*dump));
  dump->primask = primask;
  dump->basepri = __get_BASEPRI();
  dump->faultmask = __get_FAULTMASK();
  dump->control = __get_CONTROL();
  return dump;
}

#ifndef DEBUG

// The host sees the wheel unplugged until the reset: the USB peripheral stops,
// then D+ (PA12) is driven low against its pull-up. The bootloader does the
// same before DFU
static void usb_detach(void) {
  USB->CNTR = USB_CNTR_FRES | USB_CNTR_PDWN;
  RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
  USB_DP_GPIO_Port->BRR = USB_DP_Pin;
  // PA12: bits 16 to 19 of CRH. Output push-pull at 2 MHz: MODE 10, CNF 00
  USB_DP_GPIO_Port->CRH = (USB_DP_GPIO_Port->CRH & ~(0xFu << 16)) | (0x2u << 16);
}

// Makes the file around the dump, then writes it to its page. An earlier dump
// still in the page is kept: see the crash loop above
static void save_dump(void) {
  if (wheel_file_page_type(WHEEL_FILE_PAGE_DUMP) == WHEEL_FILE_DUMP) {
    if (HAL_GetTick() < FATAL_LOOP_MS) {
      bootloader_request_dfu();
    }
    return;
  }
  // The header by hand: wheel_file_make() would copy the content onto itself.
  // 208 bytes, a multiple of 4: no padding
  wheel_file_t *file = (wheel_file_t*)dump_file;
  file->header.type = WHEEL_FILE_DUMP;
  file->header.version = WHEEL_FILE_DUMP_VERSION;
  file->header.size = sizeof(wheel_file_dump_t);
  // A fatal error before MX_CRC_Init() finds the CRC unit without its clock
  RCC->AHBENR |= RCC_AHBENR_CRCEN;
  file->header.crc = wheel_file_crc(file);
  (void)wheel_file_page_write(WHEEL_FILE_PAGE_DUMP, file);
}

#endif /* DEBUG */

// Steps 3 to 6 of the top of this file
static void __attribute__((noreturn)) finish(wheel_file_dump_t *dump) {
  dump->cfsr = SCB->CFSR;
  dump->hfsr = SCB->HFSR;
  dump->mmfar = SCB->MMFAR;
  dump->bfar = SCB->BFAR;
  dump->shcsr = SCB->SHCSR;

#ifdef DEBUG
#if WHEEL_LOG_LEVEL >= 1
  wheel_log_print_dump(dump);
#endif
  __BKPT(0);
  while (1) {}
#else
  watchdog_feed_fatal();
  usb_detach();
  save_dump();
  // The erase of the page already took 20 to 40 ms: this only matters when
  // nothing was written
  for (volatile uint32_t i = 0; i < USB_DETACH_LOOPS; i++) {}
  NVIC_SystemReset();
#endif
}

/* FROM THE ENTRIES */

// A fault. The processor pushed r0-r3, r12, lr, pc and xpsr on the stack in
// use, at frame. The code comes from the exception being handled
void wheel_fatal_from_fault(const uint32_t *frame, uint32_t exc_return) {
  wheel_file_dump_t *dump = enter();
  uint32_t exception = __get_IPSR() & IPSR_ISR_Msk;

  dump->r[0] = frame[0];
  dump->r[1] = frame[1];
  dump->r[2] = frame[2];
  dump->r[3] = frame[3];
  memcpy(&dump->r[4], wheel_fatal_saved, 8u * sizeof(uint32_t));
  dump->r[12] = frame[4];
  dump->lr = frame[5];
  dump->pc = frame[6];
  dump->xpsr = frame[7];
  dump->exc_return = exc_return;
  // The sp before the fault: past the 8 words, and 4 more bytes when the
  // processor aligned the stack (bit 9 of the pushed xpsr)
  dump->sp = (uint32_t)(uintptr_t)(frame + 8) + ((frame[7] & (1u << 9)) ? 4u : 0u);

  // The code that failed: the exception it ran in, and its lr first in the
  // trace (a function that calls nothing never saves it on the stack)
  wheel_log_record_fill(&dump->error, (wheel_error_t)(exception - 1u), NULL, 0u,
                        (uint16_t)(frame[7] & IPSR_ISR_Msk), frame[5],
                        (const uint32_t*)(uintptr_t)dump->sp);
  finish(dump);
}

// WHEEL_FATAL_ERROR(). wheel_fatal_error() saved r4-r11, then the sp and the
// lr of the caller. r3 and r12 were not kept: a call does not keep them either
void wheel_fatal_from_call(wheel_error_t code, const char *file, uint16_t line) {
  wheel_file_dump_t *dump = enter();
  uint32_t caller_sp = wheel_fatal_saved[8];
  uint32_t caller_lr = wheel_fatal_saved[9];

  dump->r[0] = (uint32_t)code;
  dump->r[1] = (uint32_t)(uintptr_t)file;
  dump->r[2] = line;
  memcpy(&dump->r[4], wheel_fatal_saved, 8u * sizeof(uint32_t));
  dump->sp = caller_sp;
  dump->lr = caller_lr;
  dump->pc = caller_lr;
  dump->xpsr = __get_xPSR();

  wheel_log_record_fill(&dump->error, code, file, line,
                        (uint16_t)(__get_IPSR() & IPSR_ISR_Msk), caller_lr,
                        (const uint32_t*)(uintptr_t)caller_sp);
  finish(dump);
}
