/*
 * wheel_version.h
 *
 *  Created on: Oct 20, 2026
 *      Author: raffi
 *
 *  The one version of the firmware. Every part shares it: a release bumps it,
 *  whatever the release changed. It travels in two places:
 *
 *    bcdDevice      USB device descriptor, in BCD (usb_descriptors.c): the
 *                   host reads it before it opens the wheel
 *    file headers   version byte of every file (wheel_file.h): a file of
 *                   another version is refused
 *
 *  After an upgrade, the files of the older version are refused: the settings
 *  go back to their defaults. The application of the PC migrates them.
 *  The bootloader has a version of its own (bootloader/Makefile)
 */

#ifndef WHEEL_VERSION_H_
#define WHEEL_VERSION_H_

// From 1 to 254, one byte in a file header
#define WHEEL_VERSION 1

#if WHEEL_VERSION < 1 || WHEEL_VERSION > 254
#error "WHEEL_VERSION must be from 1 to 254"
#endif

// WHEEL_VERSION in BCD, one decimal digit per 4 bits: 12 is 0x0012, which
// Windows shows as REV_0012
#define WHEEL_VERSION_BCD                                                    \
  ((((WHEEL_VERSION / 100) % 10) << 8) | (((WHEEL_VERSION / 10) % 10) << 4) \
   | (WHEEL_VERSION % 10))

#endif /* WHEEL_VERSION_H_ */
