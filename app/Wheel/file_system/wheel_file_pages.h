/*
 * wheel_file_pages.h
 *
 *  Created on: Oct 11, 2026
 *      Author: raffi
 *
 *  Where the files are stored: the last pages of the flash, so the application
 *  has to end before WHEEL_FILE_PAGES_START (see FLASH in the linker script).
 *  A page holds its files back to back from its start: the dump page one file,
 *  the settings pages one file per group of settings (wheel_settings.h)
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_PAGES_H_
#define FILE_SYSTEM_WHEEL_FILE_PAGES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "wheel_file.h"
#include <stdbool.h>

// Counted from the end of the flash: 0 is its last page.
// The settings page is linked at its address (SETTINGS in the linker script).
// The backup page holds the same bytes: a save writes it first, so that a
// power loss always leaves one of the two valid (wheel_settings.c)
typedef enum {
  WHEEL_FILE_PAGE_SETTINGS, // the WHEEL_FILE_SETTINGS_* files
  WHEEL_FILE_PAGE_SETTINGS_BACKUP,// the same files
  WHEEL_FILE_PAGE_DUMP,// WHEEL_FILE_DUMP
  WHEEL_FILE_PAGE_COUNT
}wheel_file_page_t;

#define WHEEL_FILE_PAGE_SIZE FLASH_PAGE_SIZE
#define WHEEL_FILE_PAGE_ADDRESS(page) \
  (FLASH_BANK1_END + 1u - ((page) + 1u) * WHEEL_FILE_PAGE_SIZE)
#define WHEEL_FILE_PAGES_START WHEEL_FILE_PAGE_ADDRESS(WHEEL_FILE_PAGE_COUNT - 1u)

// Biggest content a page can hold
#define WHEEL_FILE_MAX_CONTENT_SIZE \
  (WHEEL_FILE_PAGE_SIZE - sizeof(wheel_file_header_t))

// First byte of a page that holds no file: never a type (see wheel_file.h)
#define WHEEL_FILE_PAGE_ERASED 0xFFu
#define WHEEL_FILE_PAGE_CLEARED 0x00u // see wheel_file_page_clear()

/* ACCESS */
// In wheel_file_pages.c. The flash controller is driven through its registers,
// without HAL nor tick: the fatal path (wheel_fatal.c) uses them with the
// interrupts off. Main only otherwise. The code runs from the same flash: it
// stalls until each erase (20 to 40 ms) or program (about 50 us) ends
// The type byte of the page as it is, without any check
uint8_t wheel_file_page_type(wheel_file_page_t page);

// The valid file of the page, read in place. NULL when the page holds none, or
// one that wheel_file_check() refuses. Main only: it uses the CRC unit
const wheel_file_t* wheel_file_page_read(wheel_file_page_t page);

// Erases the page, then programs file into it, the type byte last: a write cut
// by a power loss leaves no file. False if the flash reported an error
bool wheel_file_page_write(wheel_file_page_t page, const wheel_file_t *file);

// Erases the page, then programs size bytes of data from the last half-word
// to the first: the type byte of the first file comes last, and a write cut by
// a power loss leaves no first file. data is aligned on 2 bytes, size even and
// at most WHEEL_FILE_PAGE_SIZE. False if the flash reported an error, or if
// size is not allowed
bool wheel_file_page_write_bytes(wheel_file_page_t page, const void *data,
    uint32_t size);

// Writes 0x0000 over the type and the version of the file of the page, which
// then holds no file. The flash accepts 0x0000 over any value: no erase.
// False if the flash reported an error
bool wheel_file_page_clear(wheel_file_page_t page);

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_PAGES_H_ */
