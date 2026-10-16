/*
 * bootloader.c
 *
 *  Created on: Oct 17, 2026
 *      Author: raffi
 *
 *  See bootloader.h
 */

#include "bootloader.h"
#include "main.h"

// The DFU flag of the bootloader, written low word first: 0xDEADBEEFCC00FFEE
// as a 64-bit value (bootloader/reboot.h)
#define BOOTLOADER_DFU_FLAG_LOW 0xCC00FFEEu
#define BOOTLOADER_DFU_FLAG_HIGH 0xDEADBEEFu

// Top of the stack (linker script): the flag starts there
extern uint32_t _estack[];

// Volatile: nothing reads the flag again before the reset, the compiler must
// not drop the writes
void bootloader_request_dfu(void) {
  volatile uint32_t *flag = _estack;
  flag[0] = BOOTLOADER_DFU_FLAG_LOW;
  flag[1] = BOOTLOADER_DFU_FLAG_HIGH;
}

// The bootloader then sees the flag, and drives D+ low itself before DFU: the
// host enumerates the board again
void bootloader_reboot_into_dfu(void) {
  bootloader_request_dfu();
  NVIC_SystemReset();
}
