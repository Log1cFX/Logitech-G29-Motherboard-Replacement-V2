/*
 * wheel_settings.c
 *
 *  Created on: Oct 20, 2026
 *      Author: raffi
 *
 *  The settings pages and their image, see wheel_settings.h
 */

#include "wheel_settings.h"
#include "wheel_file_pages.h"
#include "wheel_log.h"

#include <stddef.h>
#include <string.h>

#define PAGE_SIZE sizeof(wheel_settings_page_t)

// Written by halfwords, read by words
static_assert(PAGE_SIZE <= WHEEL_FILE_PAGE_SIZE,
    "the settings do not fit in their page");
static_assert(PAGE_SIZE % 4u == 0u,
    "the settings page is not a multiple of 4 bytes");

// In wheel_settings_image.c
extern wheel_settings_page_t wheel_settings_image;

// From the linker script: the image in RAM, and where its bytes are loaded from
extern uint8_t _ssettings[];
extern uint8_t _esettings[];
extern uint8_t _sisettings[];

const wheel_settings_page_t *const wheel_settings = &wheel_settings_image;

// Where each file lies in the page, and its type
typedef struct {
  uint8_t type;
  uint16_t offset;
  uint16_t size; // of the content
} slot_t;

#define SLOT(field, file_type) \
  {file_type, offsetof(wheel_settings_page_t, field), \
   sizeof(((wheel_settings_page_t*)0)->field.content)}

// @formatter:off
static const slot_t slots[] = {
  SLOT(ffb, WHEEL_FILE_SETTINGS_FFB),
  SLOT(steering, WHEEL_FILE_SETTINGS_STEERING),
  SLOT(calibration, WHEEL_FILE_SETTINGS_CALIBRATION),
  SLOT(shifter, WHEEL_FILE_SETTINGS_SHIFTER),
  SLOT(pedals, WHEEL_FILE_SETTINGS_PEDALS),
  SLOT(buttons, WHEEL_FILE_SETTINGS_BUTTONS),
  SLOT(system, WHEEL_FILE_SETTINGS_SYSTEM),
  SLOT(force, WHEEL_FILE_SETTINGS_FORCE),
};
// @formatter:on

#define SLOT_COUNT (sizeof(slots) / sizeof(slots[0]))

// The file of a slot, in a page or in the image
static const wheel_file_t* file_at(const void *page, const slot_t *slot) {
  return (const wheel_file_t*)((const uint8_t*)page + slot->offset);
}

// The slot of a type, NULL when it is not a settings type
static const slot_t* slot_of(uint8_t type) {
  for (size_t i = 0; i < SLOT_COUNT; i++) {
    if (slots[i].type == type) {
      return &slots[i];
    }
  }
  return NULL;
}

// A settings page where it is in flash
static const void* page_at(wheel_file_page_t page) {
  return (const void*)WHEEL_FILE_PAGE_ADDRESS(page);
}

// True when every slot holds its file, and wheel_file_check() accepts it. The
// type and the size come first: a wrong size would make the CRC read past the
// slot, maybe past the flash
static bool page_valid(const void *page) {
  for (size_t i = 0; i < SLOT_COUNT; i++) {
    const wheel_file_t *file = file_at(page, &slots[i]);
    if (file->header.type != slots[i].type || file->header.size != slots[i].size) {
      return false;
    }
    if (wheel_file_check(file) != WHEEL_FILE_OK) {
      return false;
    }
  }
  return true;
}

// The settings page wins whenever it is valid: it was flashed, or saved to its
// end. The backup is refilled from it when it differs: after a flash, a save
// cut while the backup was written, or a corruption. Without a valid settings
// page, the backup repairs it: a save cut while the settings page was written
void wheel_settings_init(void) {
  // The linker script and this file agree on the place and the size of the page
  if ((uint32_t)(uintptr_t)_sisettings != WHEEL_FILE_PAGE_ADDRESS(
      WHEEL_FILE_PAGE_SETTINGS)
      || (size_t)(_esettings - _ssettings) != PAGE_SIZE) {
    WHEEL_FATAL_ERROR(WHEEL_ERR_SETTINGS);
  }

  const void *settings = page_at(WHEEL_FILE_PAGE_SETTINGS);
  const void *backup = page_at(WHEEL_FILE_PAGE_SETTINGS_BACKUP);
  if (page_valid(settings)) {
    if (!page_valid(backup) || memcmp(settings, backup, PAGE_SIZE) != 0) {
      // The wheel goes on without a mirror: the next start tries again
      if (!wheel_file_page_write_bytes(WHEEL_FILE_PAGE_SETTINGS_BACKUP, settings,
      PAGE_SIZE)) {
        WHEEL_LOG_ERROR(WHEEL_ERR_SETTINGS);
      }
    }
  } else if (page_valid(backup)) {
    // Checked again after the write: the image must come from a valid page
    if (!wheel_file_page_write_bytes(WHEEL_FILE_PAGE_SETTINGS, backup, PAGE_SIZE) || !page_valid(
        settings)) {
      // Tried to copy from backup and failed
      WHEEL_FATAL_ERROR(WHEEL_ERR_SETTINGS);
    }
  } else {
    // No settings at all: a reflash writes the page again
    WHEEL_FATAL_ERROR(WHEEL_ERR_SETTINGS);
  }

  // Like the startup code for .data
  memcpy(&wheel_settings_image, settings, PAGE_SIZE);
}

// The file of its slot in the image
const wheel_file_t* wheel_settings_file(uint8_t type) {
  const slot_t *slot = slot_of(type);
  return (slot != NULL) ? file_at(&wheel_settings_image, slot) : NULL;
}

// The interrupts read the image through the module pointers: they must never
// see half of a file. A file that is not a settings file of the right size is
// ignored, which wheel_file_check() already ensures
void wheel_settings_receive(const wheel_file_t *file) {
  const slot_t *slot = slot_of(file->header.type);
  if (slot == NULL || file->header.size != slot->size) {
    return;
  }
  uint8_t *place = (uint8_t*)&wheel_settings_image + slot->offset;
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  memcpy(place, file, sizeof(wheel_file_header_t) + slot->size);
  __set_PRIMASK(primask);
}

// The backup first: until the settings page is complete again, the backup is
// the valid one. A failed settings page cannot be left as it is: the next start
// repairs it from the backup
bool wheel_settings_save(void) {
  // Nothing changed since the last save: no erase, no stall
  const void *settings = page_at(WHEEL_FILE_PAGE_SETTINGS);
  const void *backup = page_at(WHEEL_FILE_PAGE_SETTINGS_BACKUP);
  if (memcmp(settings, &wheel_settings_image, PAGE_SIZE) == 0 && memcmp(
      backup, &wheel_settings_image, PAGE_SIZE)
                                                                 == 0) {
    return true;
  }
  if (!wheel_file_page_write_bytes(WHEEL_FILE_PAGE_SETTINGS_BACKUP,
                                   &wheel_settings_image, PAGE_SIZE)) {
    WHEEL_LOG_ERROR(WHEEL_ERR_SETTINGS);
    return false;
  }
  if (!wheel_file_page_write_bytes(WHEEL_FILE_PAGE_SETTINGS, &wheel_settings_image,
  PAGE_SIZE)) {
    WHEEL_FATAL_ERROR(WHEEL_ERR_SETTINGS);
  }
  return true;
}

// Only from a valid page, as at the start. The interrupts read the image
// through the module pointers: the whole page is copied before they run again
bool wheel_settings_revert(void) {
  const void *settings = page_at(WHEEL_FILE_PAGE_SETTINGS);
  if (!page_valid(settings)) {
    return false;
  }
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  memcpy(&wheel_settings_image, settings, PAGE_SIZE);
  __set_PRIMASK(primask);
  return true;
}
