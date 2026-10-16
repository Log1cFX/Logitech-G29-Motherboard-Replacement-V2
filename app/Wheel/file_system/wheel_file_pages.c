/*
 * wheel_file_pages.c
 *
 *  Created on: Oct 14, 2026
 *      Author: raffi
 *
 *  Access to the file pages, see ACCESS in wheel_file_pages.h.
 *  Flash programming of the STM32F1: RM0008, section "Flash memory", and
 *  PM0075 for the 0x0000 written over any value
 */

#include "wheel_file_pages.h"

#include <stddef.h>

// Waits for the end of the operation of the flash controller, then tells
// whether it went well. Its result flags are cleared for the next one
static bool wait_done(void) {
  while (FLASH->SR & FLASH_SR_BSY) {}
  bool ok = (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) == 0u;
  // Each flag is cleared by writing 1 to it
  FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
  return ok;
}

// Opens FLASH->CR for writing, then ends an operation left running: the fatal
// path may have stopped main in the middle of one
static void unlock(void) {
  if (FLASH->CR & FLASH_CR_LOCK) {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }
  (void)wait_done();
  FLASH->CR = 0u;
}

// Closes FLASH->CR again: a stray write can no longer change the flash
static void lock(void) {
  FLASH->CR = FLASH_CR_LOCK;
}

// Programs one half-word: an erased one, or any one with 0x0000
static bool program_halfword(uint32_t address, uint16_t value) {
  FLASH->CR = FLASH_CR_PG;
  *(volatile uint16_t*)address = value;
  bool ok = wait_done();
  FLASH->CR = 0u;
  return ok;
}

// Sets every byte of the page that starts at address to 0xFF
static bool erase_page(uint32_t address) {
  FLASH->CR = FLASH_CR_PER;
  FLASH->AR = address;
  FLASH->CR = FLASH_CR_PER | FLASH_CR_STRT;
  bool ok = wait_done();
  FLASH->CR = 0u;
  return ok;
}

// Read as volatile: a program of this file may have changed it since the last
// read
uint8_t wheel_file_page_type(wheel_file_page_t page) {
  return *(volatile const uint8_t*)WHEEL_FILE_PAGE_ADDRESS(page);
}

// The file of the page when wheel_file_check() accepts it, NULL otherwise
const wheel_file_t* wheel_file_page_read(wheel_file_page_t page) {
  const wheel_file_t *file = (const wheel_file_t*)WHEEL_FILE_PAGE_ADDRESS(page);
  uint8_t type = file->header.type;
  // Erased or cleared: no file. A size over the page would make the CRC read
  // past its end
  if (type == WHEEL_FILE_PAGE_ERASED || type == WHEEL_FILE_PAGE_CLEARED
      || file->header.size > WHEEL_FILE_MAX_CONTENT_SIZE) {
    return NULL;
  }
  return (wheel_file_check(file) == WHEEL_FILE_OK) ? file : NULL;
}

// Erase, then the half-words of the file from its content back to its first
// one. The first half-word holds the type: until it is written, the page
// reads as erased
bool wheel_file_page_write(wheel_file_page_t page, const wheel_file_t *file) {
  if (file->header.size > WHEEL_FILE_MAX_CONTENT_SIZE) {
    return false;
  }
  uint32_t address = WHEEL_FILE_PAGE_ADDRESS(page);
  const uint16_t *halfwords = (const uint16_t*)file;
  uint32_t count = WHEEL_FILE_TOTAL_SIZE(file->header.size) / 2u;

  unlock();
  bool ok = erase_page(address);
  // From the last half-word down to the first, while nothing failed
  for (uint32_t i = count; ok && i > 0u; i--) {
    ok = program_halfword(address + (i - 1u) * 2u, halfwords[i - 1u]);
  }
  lock();
  return ok;
}

// One half-word: the type and the version, which wheel_file_page_read() then
// takes for a cleared page
bool wheel_file_page_clear(wheel_file_page_t page) {
  unlock();
  bool ok = program_halfword(WHEEL_FILE_PAGE_ADDRESS(page), 0x0000u);
  lock();
  return ok;
}
