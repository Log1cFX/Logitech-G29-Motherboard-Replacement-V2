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

inline float clip01(float v) {
    if (v < 0.0f)  return 0.0f;
    if (v > 0.5f)  return 0.5f;
    return v;
}

} /* anonymous namespace */

namespace ffb {

Biquad::Biquad() {
    z1 = z2 = 0.0f;
}

Biquad::Biquad(BiquadType t, float fc, float q, float peakGainDB) {
    setBiquad(t, fc, q, peakGainDB);
}

void Biquad::setFc(float fc) {
    fc = clip01(fc);
    this->Fc = fc;
    calcBiquad();
}

float Biquad::getFc() const { return this->Fc; }

void Biquad::setQ(float q) {
    this->Q = q;
    calcBiquad();
}

float Biquad::getQ() const { return this->Q; }

float Biquad::process(float in) {
    float out = in * a0 + z1;
    z1 = in * a1 + z2 - b1 * out;
    z2 = in * a2 - b2 * out;
    return out;
}

void Biquad::setBiquad(BiquadType t, float fc, float q, float peakGainDB) {
    fc = clip01(fc);
    this->type = t;
    this->Q = q;
    this->Fc = fc;
    this->peakGain = peakGainDB;
    calcBiquad();
}

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
