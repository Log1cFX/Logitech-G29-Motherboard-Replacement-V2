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

    float process(float in);
    void  setBiquad(BiquadType type, float Fc, float Q, float peakGain);
    void  setFc(float Fc);       /* Fc is normalised: f / samplerate, must be < 0.5 */
    float getFc() const;
    void  setQ(float Q);
    float getQ() const;
    void  calcBiquad();

protected:
    BiquadType type = BiquadType::lowpass;
    float a0 = 0, a1 = 0, a2 = 0, b1 = 0, b2 = 0;
    float Fc = 0, Q = 0, peakGain = 0;
    float z1 = 0, z2 = 0;
};

} /* namespace ffb */

#endif /* FFB_BIQUAD_H_ */
