/*
 * wheel_file_pages.h
 *
 *  Created on: Oct 11, 2026
 *      Author: raffi
 *
 *  Where the files are stored: one file per flash page, at the start of the
 *  page. The pages are the last ones of the flash, so the application has to
 *  end before WHEEL_FILE_PAGES_START (see FLASH in the linker script)
 */

#ifndef FILE_SYSTEM_WHEEL_FILE_PAGES_H_
#define FILE_SYSTEM_WHEEL_FILE_PAGES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "wheel_file.h"

// Counted from the end of the flash: 0 is its last page.
// The settings use their two pages in turn: a save writes the file to the
// erased page, then erases the page of the previous file. A power loss in
// between leaves two valid files: save_count (wheel_file_settings.h) tells
// which one is current
typedef enum {
  WHEEL_FILE_PAGE_SETTINGS_1, // WHEEL_FILE_SETTINGS
  WHEEL_FILE_PAGE_SETTINGS_2, // WHEEL_FILE_SETTINGS
  WHEEL_FILE_PAGE_DUMP, // WHEEL_FILE_DUMP
  WHEEL_FILE_PAGE_COUNT
} wheel_file_page_t;

#define WHEEL_FILE_PAGE_SIZE FLASH_PAGE_SIZE
#define WHEEL_FILE_PAGE_ADDRESS(page) \
  (FLASH_BANK1_END + 1u - ((page) + 1u) * WHEEL_FILE_PAGE_SIZE)
#define WHEEL_FILE_PAGES_START WHEEL_FILE_PAGE_ADDRESS(WHEEL_FILE_PAGE_COUNT - 1u)

// Biggest content a page can hold
#define WHEEL_FILE_MAX_CONTENT_SIZE \
  (WHEEL_FILE_PAGE_SIZE - sizeof(wheel_file_header_t))

#ifdef __cplusplus
}
#endif

#endif /* FILE_SYSTEM_WHEEL_FILE_PAGES_H_ */
