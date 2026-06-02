/*
 * ffb_effect.h
 *
 * Internal effect-storage struct. Equivalent to OpenFFBoard's FFB_Effect
 * but with the per-axis Biquad filter slots inlined (no std::unique_ptr,
 * no heap allocation). Logic that touches it must use filter_active[axis]
 * to know whether the filter slot is in use, instead of a null pointer check.
 */

#ifndef FFB_EFFECT_H_
#define FFB_EFFECT_H_

#include <cstdint>

#include "ffb/ffb_biquad.h"
#include "ffb/ffb_config.h"
#include "ffb/ffb_defs.h"

namespace ffb {

struct Effect {
    /* Lifecycle: 0=inactive (free), 1=running. Set by the parser when the
     * host issues an Effect Operation report. Marked volatile because it
     * is written by the USB callback and read by the calculator. */
    volatile uint8_t state = 0;

    uint8_t  type     = FFB_EFFECT_NONE;
    int16_t  offset   = 0;
    uint8_t  gain     = 255;
    int16_t  magnitude = 0;
    int16_t  startLevel = 0;        /* Ramp                          */
    int16_t  endLevel   = 0;        /* Ramp                          */

    /* Direction unit vector per axis. Built by the parser from the host's
     * direction/enableAxis fields. -1..1 range. */
    float    axisMagnitudes[FFB_MAX_AXIS] = {0};

    FFB_Effect_Condition conditions[FFB_MAX_AXIS];

    int16_t  phase      = 0;
    uint16_t period     = 0;
    uint32_t duration   = FFB_EFFECT_DURATION_INFINITE;
    uint16_t attackLevel= 0;
    uint16_t fadeLevel  = 0;
    uint32_t attackTime = 0;
    uint32_t fadeTime   = 0;

    /* Inline filter storage. The original code used unique_ptr<Biquad>;
     * here the slot is always present and filter_active[axis] tracks
     * whether setFilters() has initialised it. */
    Biquad   filter[FFB_MAX_AXIS];
    bool     filter_active[FFB_MAX_AXIS] = {false};

    uint16_t startDelay  = 0;
    uint32_t startTime   = 0;       /* ms tick at which effect started */
    uint16_t samplePeriod= 0;
    bool     useEnvelope = false;
    bool     useSingleCondition = true;
};

} /* namespace ffb */

#endif /* FFB_EFFECT_H_ */
