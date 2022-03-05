#ifndef SST_FILTERS_DIODELADDER_H
#define SST_FILTERS_DIODELADDER_H

#include "sst/utilities/basic_dsp.h"
#include "QuadFilterUnit.h"
#include "FilterCoefficientMaker.h"

/**
 * This namespace contains an adaptation of the filter from
 * https://github.com/TheWaveWarden/odin2/blob/master/Source/audio/Filters/DiodeFilter.cpp
 */
namespace sst::filters::DiodeLadderFilter
{
template <typename TuningProvider>
static float clampedFrequency(float pitch, float sampleRate, TuningProvider *provider)
{
    auto freq =
        provider->note_to_pitch_ignoring_tuning(pitch + 69) * (float)TuningProvider::MIDI_0_FREQ;
    freq = utilities::limit_range(freq, 5.f, sampleRate * 0.3f);
    return freq;
}

#define F(a) _mm_set_ps1(a)
#define M(a, b) _mm_mul_ps(a, b)
#define D(a, b) _mm_div_ps(a, b)
#define A(a, b) _mm_add_ps(a, b)
#define S(a, b) _mm_sub_ps(a, b)
// reciprocal
#define reci(a) _mm_rcp_ps(a)

static inline __m128 getFO(const __m128 beta, const __m128 delta, const __m128 feedback,
                           const __m128 z) noexcept
{
    // (feedback * delta + z) * beta
    return M(A(M(feedback, delta), z), beta);
}

// @TODO: it looks like the `beta` and `delta` arguments are not being used?
static inline __m128 doLpf(const __m128 input, const __m128 alpha, const __m128 beta,
                           const __m128 gamma, const __m128 delta, const __m128 epsilon,
                           const __m128 ma0, const __m128 feedback, const __m128 feedback_output,
                           __m128 &z) noexcept
{
    // input * gamma + feedback + epsilon * feedback_output
    const __m128 i = A(A(M(input, gamma), feedback), M(epsilon, feedback_output));
    const __m128 v = M(S(M(ma0, i), z), alpha);
    const __m128 result = A(v, z);
    z = A(v, result);
    return result;
}

// can't fit all the coefficients in the 8-coefficient limit, so we have to compute a lot of
// stuff per sample q_q
enum dlf_coeffs
{
    dlf_alpha = 0,
    dlf_gamma,
    dlf_g,
    dlf_G4,
    dlf_G3,
    dlf_G2,
    dlf_G1,
    dlf_km, // k_modded
};

enum dlf_state
{
    dlf_z1, // z-1 state for LPF 1
    dlf_z2, // LPF2
    dlf_z3, // ...
    dlf_z4,
    dlf_feedback3, // feedback for LPF3 (feedback for LPF4 is 0)
    dlf_feedback2,
    dlf_feedback1,
};

template <typename TuningProvider>
void makeCoefficients(FilterCoefficientMaker<TuningProvider> *cm, float freq, float reso,
                      float sampleRate, float sampleRateInv, TuningProvider *provider)
{
    const float wd = clampedFrequency(freq, sampleRate, provider) * 2.0f * (float)M_PI;
    const float wa = (2.0f * sampleRate) * utilities::DSP::fasttan(wd * sampleRateInv * 0.5f);
    const float g = wa * sampleRateInv * 0.5f;

    const float G4 = 0.5f * g / (1.0f + g);
    const float G3 = 0.5f * g / (1.0f + g - 0.5f * g * G4);
    const float G2 = 0.5f * g / (1.0f + g - 0.5f * g * G3);
    const float G1 = g / (1.0f + g - g * G2);
    const float m_gamma = G4 * G3 * G2 * G1;

    const float G = g / (1.0f + g);

    const float k = reso * 16.0f;
    // clamp to [0..16]
    const float km = (k > 16.f) ? 16.f : ((k < 0.f) ? 0.f : k);

    float C[n_cm_coeffs];
    C[dlf_alpha] = G;
    C[dlf_gamma] = m_gamma;
    C[dlf_g] = g;
    C[dlf_G4] = G4;
    C[dlf_G3] = G3;
    C[dlf_G2] = G2;
    C[dlf_G1] = G1;
    C[dlf_km] = km;

    cm->FromDirect(C);
}

template <FilterSubType subtype>
inline __m128 process(QuadFilterUnitState *__restrict f, __m128 input)
{
    for (int i = 0; i < n_cm_coeffs; ++i)
    {
        f->C[i] = A(f->C[i], f->dC[i]);
    }

    // hopefully the optimiser will take care of the duplicatey bits

    const __m128 zero = F(0.0f);
    const __m128 one = F(1.0f);
    const __m128 half = F(0.5f);

    const __m128 sg3 = f->C[dlf_G4];
    const __m128 sg2 = M(sg3, f->C[dlf_G3]);
    const __m128 sg1 = M(sg2, f->C[dlf_G2]);
    // sg4 is 1.0, just inline it

    const __m128 g = f->C[dlf_g];
    // g plus one, common so do it only once
    const __m128 gp1 = A(g, one);
    // half of g
    const __m128 hg = M(f->C[dlf_g], half);

    // 1.0 / (gp1 - g * G2)
    const __m128 beta1 = reci(S(gp1, M(g, f->C[dlf_G2])));
    // 1.0 / (gp1 - g * 0.5 * G3
    const __m128 beta2 = reci(S(gp1, M(hg, f->C[dlf_G3])));
    // 1.0 / (gp1 - g * 0.5 * G4
    const __m128 beta3 = reci(S(gp1, M(hg, f->C[dlf_G4])));
    // 1.0 / gp1
    const __m128 beta4 = reci(gp1);

    // nothing to compute for deltas, inline them

    // G1 * G2 + 1.0
    const __m128 gamma1 = A(M(f->C[dlf_G1], f->C[dlf_G2]), one);
    // G2 * G3 + 1.0
    const __m128 gamma2 = A(M(f->C[dlf_G2], f->C[dlf_G3]), one);
    // G3 * G4 + 1.0
    const __m128 gamma3 = A(M(f->C[dlf_G3], f->C[dlf_G4]), one);
    // gamma4 is always 1.0, just inline it

    // nothing to compute for epsilons or ma0, inline them

    // feedback4 is always zero, inline it
    const __m128 feedback3 = getFO(beta4, zero, zero, f->R[dlf_z4]);
    const __m128 feedback2 = getFO(beta3, hg, f->R[dlf_feedback3], f->R[dlf_z3]);
    const __m128 feedback1 = getFO(beta2, hg, f->R[dlf_feedback2], f->R[dlf_z2]);

    const __m128 sigma = A(A(A(M(sg1, getFO(beta1, g, feedback1, f->R[dlf_z1])),
                               M(sg2, getFO(beta2, hg, feedback2, f->R[dlf_z2]))),
                             M(sg3, getFO(beta3, hg, feedback3, f->R[dlf_z3]))),
                           M(one, getFO(beta4, zero, zero, f->R[dlf_z4])));

    f->R[dlf_feedback3] = feedback3;
    f->R[dlf_feedback2] = feedback2;
    f->R[dlf_feedback1] = feedback1;

    // gain compensation
    const __m128 comp = M(A(M(F(0.3f), f->C[dlf_km]), one), input);

    // (comp - km * sigma) / (km * gamma + 1.0)
    const __m128 u = D(S(comp, M(f->C[dlf_km], sigma)), A(M(f->C[dlf_km], f->C[dlf_gamma]), one));

    const __m128 result1 = doLpf(u, f->C[dlf_alpha], beta1, gamma1, g, f->C[dlf_G2], one, feedback1,
                                 getFO(beta1, g, feedback1, f->R[dlf_z1]), f->R[dlf_z1]);
    const __m128 result2 =
        doLpf(result1, f->C[dlf_alpha], beta2, gamma2, hg, f->C[dlf_G3], half, feedback2,
              getFO(beta2, hg, feedback2, f->R[dlf_z2]), f->R[dlf_z2]);
    const __m128 result3 =
        doLpf(result2, f->C[dlf_alpha], beta3, gamma3, hg, f->C[dlf_G4], half, feedback3,
              getFO(beta3, hg, feedback3, f->R[dlf_z3]), f->R[dlf_z3]);
    const __m128 result4 = doLpf(result3, f->C[dlf_alpha], beta4, one, zero, zero, half, zero,
                                 getFO(beta4, zero, zero, f->R[dlf_z4]), f->R[dlf_z4]);

    // Just like in QuadFilterUnit.cpp/LPMOOGquad, it's fine for the whole quad to return the same
    // subtype because integer parameters like f->WP are not modulatable and QuadFilterUnit is only
    // parallel across voices, so it would have been the same for each part of the quad anyway.
    switch (subtype)
    {
    case st_diode_6dB:
        return M(result1, F(0.125f)); // 6dB/oct
    case st_diode_12dB:
        return M(result2, F(0.3f)); // 12dB/oct
    case st_diode_18dB:
        return M(result3, F(0.6f)); // 18dB/oct
    case st_diode_24dB:
    default:
        return M(result4, F(1.2f)); // 24dB/oct
    }
}

#undef F
#undef M
#undef D
#undef A
#undef S
#undef reci

} // namespace sst::filters::DiodeLadderFilter

#endif // SST_FILTERS_DIODELADDER_H
