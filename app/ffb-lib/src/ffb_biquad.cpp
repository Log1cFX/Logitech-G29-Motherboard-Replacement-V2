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
 * ffb_biquad.cpp
 *
 * Ported verbatim from OpenFFBoard Filters.cpp. The only change is that
 * clip<float,float>(...) was inlined as a local helper to drop the
 * cppmain.h dependency.
 */

#include "ffb/ffb_biquad.h"

#include <cmath>

#ifndef M_PI
#  define M_PI 3.14159265358979323846f
#endif

namespace {

/* Inlined replacement for OpenFFBoard's clip<float>(): clamp the normalised
 * cutoff to [0, 0.5]. A biquad's normalised frequency (f / samplerate) must
 * stay below Nyquist (0.5) or the filter becomes unstable. */
inline float clip01(float v) {
    if (v < 0.0f)  return 0.0f;
    if (v > 0.5f)  return 0.5f;
    return v;
}

} /* anonymous namespace */

namespace ffb {

/* Default ctor: an unconfigured filter with a cleared delay line. Call
 * setBiquad()/setFc()/setQ() before relying on it to actually filter. */
Biquad::Biquad() {
    z1 = z2 = 0.0f;
}

/* Convenience ctor: configure the filter immediately. */
Biquad::Biquad(BiquadType t, float fc, float q, float peakGainDB) {
    setBiquad(t, fc, q, peakGainDB);
}

/* Change the normalised cutoff (f / samplerate) and recompute coefficients. */
void Biquad::setFc(float fc) {
    fc = clip01(fc);
    this->Fc = fc;
    calcBiquad();
}

float Biquad::getFc() const { return this->Fc; }

/* Change the Q factor and recompute coefficients. */
void Biquad::setQ(float q) {
    this->Q = q;
    calcBiquad();
}

float Biquad::getQ() const { return this->Q; }

/* Run one sample through the filter and return the output. This is the
 * transposed direct-form-II difference equation: a0..a2 are the feed-forward
 * (numerator) coefficients, b1/b2 the feedback (denominator) ones, and z1/z2
 * are the two-sample delay line carried between calls. Call once per tick. */
float Biquad::process(float in) {
    float out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return out;
}

/* Configure type, cutoff, Q and peak gain in one go, then build coefficients.
 * peakGainDB only matters for the peak/shelf types; the FFB engine only ever
 * uses lowpass, where it is ignored. */
void Biquad::setBiquad(BiquadType t, float fc, float q, float peakGainDB) {
    fc = clip01(fc);
    this->type = t;
    this->Q = q;
    this->Fc = fc;
    this->peakGain = peakGainDB;
    calcBiquad();
}

/* Recompute the five coefficients (a0,a1,a2,b1,b2) from the current type,
 * cutoff, Q and peak gain, and reset the delay line. The closed-form
 * expressions below are the standard earlevel/RBJ biquad cookbook, one branch
 * per response type. The FFB engine only uses `lowpass`; the other cases are
 * kept verbatim from upstream for completeness.
 *   K = tan(pi * Fc) is the bilinear-transform frequency pre-warp;
 *   V is the linear peak gain (10^(|dB|/20)), used only by peak/shelf. */
void Biquad::calcBiquad() {
    z1 = 0.0f;
    z2 = 0.0f;
    float norm;
    float V = std::pow(10.0f, std::fabs(peakGain) / 20.0f);
    float K = std::tan(static_cast<float>(M_PI) * Fc);
    switch (this->type) {
        case BiquadType::lowpass:
            norm = 1 / (1 + K / Q + K * K);
            a0 = K * K * norm;
            a1 = 2 * a0;
            a2 = a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case BiquadType::highpass:
            norm = 1 / (1 + K / Q + K * K);
            a0 = 1 * norm;
            a1 = -2 * a0;
            a2 = a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case BiquadType::bandpass:
            norm = 1 / (1 + K / Q + K * K);
            a0 = K / Q * norm;
            a1 = 0;
            a2 = -a0;
            b1 = 2 * (K * K - 1) * norm;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case BiquadType::notch:
            norm = 1 / (1 + K / Q + K * K);
            a0 = (1 + K * K) * norm;
            a1 = 2 * (K * K - 1) * norm;
            a2 = a0;
            b1 = a1;
            b2 = (1 - K / Q + K * K) * norm;
            break;

        case BiquadType::peak:
            if (peakGain >= 0) {            /* boost */
                norm = 1 / (1 + 1/Q * K + K * K);
                a0 = (1 + V/Q * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - V/Q * K + K * K) * norm;
                b1 = a1;
                b2 = (1 - 1/Q * K + K * K) * norm;
            } else {                         /* cut */
                norm = 1 / (1 + V/Q * K + K * K);
                a0 = (1 + 1/Q * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - 1/Q * K + K * K) * norm;
                b1 = a1;
                b2 = (1 - V/Q * K + K * K) * norm;
            }
            break;

        case BiquadType::lowshelf:
            if (peakGain >= 0) {            /* boost */
                norm = 1 / (1 + std::sqrt(2.0f) * K + K * K);
                a0 = (1 + std::sqrt(2.0f*V) * K + V * K * K) * norm;
                a1 = 2 * (V * K * K - 1) * norm;
                a2 = (1 - std::sqrt(2.0f*V) * K + V * K * K) * norm;
                b1 = 2 * (K * K - 1) * norm;
                b2 = (1 - std::sqrt(2.0f) * K + K * K) * norm;
            } else {                         /* cut */
                norm = 1 / (1 + std::sqrt(2.0f*V) * K + V * K * K);
                a0 = (1 + std::sqrt(2.0f) * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - std::sqrt(2.0f) * K + K * K) * norm;
                b1 = 2 * (V * K * K - 1) * norm;
                b2 = (1 - std::sqrt(2.0f*V) * K + V * K * K) * norm;
            }
            break;

        case BiquadType::highshelf:
            if (peakGain >= 0) {            /* boost */
                norm = 1 / (1 + std::sqrt(2.0f) * K + K * K);
                a0 = (V + std::sqrt(2.0f*V) * K + K * K) * norm;
                a1 = 2 * (K * K - V) * norm;
                a2 = (V - std::sqrt(2.0f*V) * K + K * K) * norm;
                b1 = 2 * (K * K - 1) * norm;
                b2 = (1 - std::sqrt(2.0f) * K + K * K) * norm;
            } else {                         /* cut */
                norm = 1 / (V + std::sqrt(2.0f*V) * K + K * K);
                a0 = (1 + std::sqrt(2.0f) * K + K * K) * norm;
                a1 = 2 * (K * K - 1) * norm;
                a2 = (1 - std::sqrt(2.0f) * K + K * K) * norm;
                b1 = 2 * (K * K - V) * norm;
                b2 = (V - std::sqrt(2.0f*V) * K + K * K) * norm;
            }
            break;
    }
}

} /* namespace ffb */
