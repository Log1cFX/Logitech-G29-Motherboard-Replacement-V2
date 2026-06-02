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
#  define FFB_MAX_AXIS 1
#endif

#if FFB_MAX_AXIS < 1 || FFB_MAX_AXIS > 3
#  error "FFB_MAX_AXIS must be 1, 2, or 3"
#endif

/* Number of simultaneous effects the device can hold. The host's PID
 * pool report advertises this value; 40 is the OpenFFBoard default. */
#ifndef FFB_MAX_EFFECTS
#  define FFB_MAX_EFFECTS 20
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
