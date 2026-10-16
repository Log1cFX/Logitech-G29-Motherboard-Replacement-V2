/*
 * bootloader.h
 *
 *  Created on: Oct 17, 2026
 *      Author: raffi
 *
 *  The DFU bootloader of the board (bootloader/ in the repository): what the
 *  firmware can ask it for the next start. It reads a flag in the 8 bytes of
 *  RAM right above _estack, which both linker scripts keep free
 *  (bootloader/reboot.h). A reset keeps the flag, a power cycle loses it, and
 *  the bootloader clears it before it starts the firmware
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#ifdef __cplusplus
extern "C" {
#endif

// At the next reset, the bootloader stays in DFU mode instead of starting the
// firmware. Any context
void bootloader_request_dfu(void);

// bootloader_request_dfu(), then a software reset. Any context, never returns
void bootloader_reboot_into_dfu(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_H_ */
