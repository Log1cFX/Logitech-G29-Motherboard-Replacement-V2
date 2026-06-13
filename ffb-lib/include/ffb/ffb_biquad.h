/*
 * SPDX-License-Identifier: MIT
 *
 * MIT License
 *
 * Copyright (c) 2020 Yannick Richter (OpenFFBoard project and contributors)
 * Copyright (c) 2026 Santryan Raffi
 *
 * Derived from OpenFFBoard (https://github.com/Ultrawipf/OpenFFBoard).
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
 * ffb_biquad.h
 *
 * Direct-form-I biquad filter. Math copied verbatim from
 * https://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
 *
 * Ported from OpenFFBoard's Filters.h/cpp with the only change being
 * an inline clamp helper (was clip<>() in cppmain.h) so the file has
 * no project-specific dependencies.
 */

#ifndef FFB_BIQUAD_H_
#define FFB_BIQUAD_H_

#include <cstdint>

namespace ffb {

/* Frequency in Hz, q in float q*100 (so Q=0.5 -> q=50). */
struct biquad_constant_t {
    uint16_t freq;
    uint8_t  q;
};

enum class BiquadType : uint8_t {
    lowpass = 0,
    highpass,
    bandpass,
    notch,
    peak,
    lowshelf,
    highshelf
};

class Biquad {
public:
    Biquad();
    Biquad(BiquadType type, float Fc, float Q, float peakGainDB);

    float process(float in);     /* filter one sample; call once per tick        */
    void  setBiquad(BiquadType type, float Fc, float Q, float peakGain); /* configure + build coeffs */
    void  setFc(float Fc);       /* Fc is normalised: f / samplerate, must be < 0.5 */
    float getFc() const;
    void  setQ(float Q);
    float getQ() const;
    void  calcBiquad();          /* rebuild coefficients from type/Fc/Q/peakGain */

protected:
    BiquadType type = BiquadType::lowpass;
    float a0 = 0, a1 = 0, a2 = 0;  /* feed-forward (numerator) coefficients     */
    float b1 = 0, b2 = 0;          /* feedback (denominator) coefficients        */
    float Fc = 0;                  /* normalised cutoff, f / samplerate (< 0.5)  */
    float Q  = 0;                  /* quality factor                             */
    float peakGain = 0;            /* peak/shelf gain in dB (lowpass ignores it) */
    float z1 = 0, z2 = 0;          /* two-sample delay line (filter state)       */
};

} /* namespace ffb */

#endif /* FFB_BIQUAD_H_ */
