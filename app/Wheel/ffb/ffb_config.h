/*
 * SPDX-License-Identifier: MIT
 *
 * MIT License
 *
 * Copyright (c) 2026 Santryan Raffi
 *
 * Part of a standalone force-feedback library derived from OpenFFBoard
 * (https://github.com/Ultrawipf/OpenFFBoard).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * ffb_config.h
 *
 * Compile-time options of the ffb library for this project. The library reads
 * them through ffb_options.h, which documents each option and its default.
 */

#ifndef FFB_CONFIG_H_
#define FFB_CONFIG_H_

#include "common_types.h" // DBG

#ifndef FFB_MAX_AXIS
#  define FFB_MAX_AXIS 1
#endif

#if FFB_MAX_AXIS < 1 || FFB_MAX_AXIS > 3
#  error "FFB_MAX_AXIS must be 1, 2, or 3"
#endif

// Effect slots advertised to the host. 40 is the OpenFFBoard default
#ifndef FFB_MAX_EFFECTS
#  define FFB_MAX_EFFECTS 40
#endif

// Rate of ffb_calculate(). The firmware never calls ffb_set_samplerate(), so
// keep it equal to CONTROL_LOOP_RATE_HZ (wheel_def.h)
#ifndef FFB_DEFAULT_SAMPLERATE_HZ
#  define FFB_DEFAULT_SAMPLERATE_HZ 1000.0f
#endif

// Added to every HID report id of the library. 0 matches the OpenFFBoard
// descriptor
#ifndef FFB_ID_OFFSET
#  define FFB_ID_OFFSET 0
#endif

// The logs of the library go to the debug console
#define FFB_LOG(...)  DBG(__VA_ARGS__)

#endif /* FFB_CONFIG_H_ */
