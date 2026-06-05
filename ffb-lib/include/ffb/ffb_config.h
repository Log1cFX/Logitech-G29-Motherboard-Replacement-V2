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
 * Compile-time configuration for the standalone FFB library.
 *
 * Users may override any of these by defining them on the compiler
 * command line (-DFFB_MAX_AXIS=1) or in a project-wide header included
 * before any ffb header.
 */

#ifndef FFB_CONFIG_H_
#define FFB_CONFIG_H_

/* Number of physical axes the device exposes. Must be 1, 2, or 3. */
#ifndef FFB_MAX_AXIS
#  define FFB_MAX_AXIS 2
#endif

#if FFB_MAX_AXIS < 1 || FFB_MAX_AXIS > 3
#  error "FFB_MAX_AXIS must be 1, 2, or 3"
#endif

/* Number of simultaneous effects the device can hold. The host's PID
 * pool report advertises this value; 40 is the OpenFFBoard default. */
#ifndef FFB_MAX_EFFECTS
#  define FFB_MAX_EFFECTS 40
#endif

/* Default effect-calculation rate, in Hz. Used to initialise biquad
 * filter coefficients before the user calls setSamplerate(). */
#ifndef FFB_DEFAULT_SAMPLERATE_HZ
#  define FFB_DEFAULT_SAMPLERATE_HZ 1000.0f
#endif

/* Offset added to every HID report ID before transmission. 0 matches
 * the OpenFFBoard descriptor; advanced users with composite HID stacks
 * may shift the IDs to avoid collisions. */
#ifndef FFB_ID_OFFSET
#  define FFB_ID_OFFSET 0
#endif

/* Optional debug log hook. Define this to your own logging function
 * before including any ffb header to capture effect lifecycle events. */
#ifndef FFB_LOG
#  define FFB_LOG(msg) ((void)0)
#endif

#endif /* FFB_CONFIG_H_ */
