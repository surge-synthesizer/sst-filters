/*
 * sst-filters - A header-only collection of SIMD filter
 * implementations by the Surge Synth Team
 *
 * Copyright 2019-2025, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-filters is released under the Gnu General Public Licens
 * version 3 or later. Some of the filters in this package
 * originated in the version of Surge open sourced in 2018.
 *
 * All source in sst-filters available at
 * https://github.com/surge-synthesizer/sst-filters
 */

#ifndef INCLUDE_SST_FILTERS_CYTOMICSVFQUADFORM_H
#define INCLUDE_SST_FILTERS_CYTOMICSVFQUADFORM_H

#include "QuadFilterUnit.h"

namespace sst::filters::cytomic_quadform
{

struct Coeff
{
    // not an enum to avoid annoying casts
    static constexpr int a1 = 0;
    static constexpr int a2 = 1;
    static constexpr int a3 = 2;
    static constexpr int m0 = 3;
    static constexpr int m1 = 4;
    static constexpr int m2 = 5;
};

struct Reg
{
    static constexpr int ic1eq = 0;
    static constexpr int ic2eq = 1;
};

template <typename TuningProvider>
void makeCoefficients(FilterCoefficientMaker<TuningProvider> *cm, float freq, float res,
                      int subtype, float sampleRate, float sampleRateInv, TuningProvider *provider,
                      float bellShelfAmp)
{
    float lC[n_cm_coeffs]{};

    auto ufr = 440 * FilterCoefficientMaker<TuningProvider>::provider_note_to_pitch_ignoring_tuning(
                         provider, freq);
    auto conorm = std::clamp(ufr * sampleRateInv, 0.f, 0.499f); // stable until nyquist
    static float lf{-1000}, lr{-1000};

    lf = freq;
    lr = res;
    res = std::clamp(res, 0.f, 0.99f);

    auto g = sst::basic_blocks::dsp::fasttan(M_PI * conorm);
    auto k = 2.0 - 2 * res;

    if (subtype == st_cytomic_bell)
    {
        bellShelfAmp = std::max(bellShelfAmp, 0.001f);
        k = k / bellShelfAmp;
    }

    auto gk = g + k;
    lC[Coeff::a1] = 1.0 / (1 + g * gk);
    lC[Coeff::a2] = g * lC[Coeff::a1];
    lC[Coeff::a3] = g * lC[Coeff::a2];

    switch (subtype)
    {
    case st_cytomic_lp:
        // The lowpass opt below hardcodes these so we can skip this assignment
        // lC[Coeff::m0] = 0.0;
        // lC[Coeff::m1] = 0.0;
        // lC[Coeff::m2] = 1.0;
        break;
    case st_cytomic_bp:
        lC[Coeff::m0] = 0.0;
        lC[Coeff::m1] = 1.0;
        lC[Coeff::m2] = 0.0;
        break;
    case st_cytomic_hp:
        lC[Coeff::m0] = 1.0;
        lC[Coeff::m1] = -k;
        lC[Coeff::m2] = -1;
        break;
    case st_cytomic_notch:
        lC[Coeff::m0] = 1.0;
        lC[Coeff::m1] = -k;
        lC[Coeff::m2] = 0;
        break;
    case st_cytomic_peak:
        lC[Coeff::m0] = 1.0;
        lC[Coeff::m1] = -k;
        lC[Coeff::m2] = -2;
        break;
    case st_cytomic_allpass:
        lC[Coeff::m0] = 1.0;
        lC[Coeff::m1] = -2 * k;
        lC[Coeff::m2] = 0;
        break;
    case st_cytomic_bell:
    {
        auto A = std::clamp(bellShelfAmp, 0.001f, 0.999f);
        lC[Coeff::m0] = 1.0;
        lC[Coeff::m1] = k * (A * A - 1);
        lC[Coeff::m2] = 0;
    }
    break;
    case st_cytomic_lowshelf:
    {
        auto A = std::clamp(bellShelfAmp, 0.001f, 0.999f);
        lC[Coeff::m0] = 1.0;
        lC[Coeff::m1] = k * (A - 1);
        lC[Coeff::m2] = A * A - 1;
    }
    break;
    case st_cytomic_highshelf:
    {
        auto A = std::clamp(bellShelfAmp, 0.001f, 0.999f);
        lC[Coeff::m0] = A * A;
        lC[Coeff::m1] = A * k * (1 - A);
        lC[Coeff::m2] = 1 - A * A;
    }
    break;
    }

    cm->FromDirect(lC);
}

#define ADD(a, b) SIMD_MM(add_ps)(a, b)
#define SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define DIV(a, b) SIMD_MM(div_ps)(a, b)
#define MUL(a, b) SIMD_MM(mul_ps)(a, b)

// a very common case is low pas which has m0=m1=0 and m2=1 so hardcode that in.
template <bool lowPassOpt = false>
inline SIMD_M128 CytomicQuad(QuadFilterUnitState *__restrict f, SIMD_M128 vin)
{
    if (lowPassOpt) // m0 == m1 == m2 == 0
    {
        for (int i = 0; i <= Coeff::a3; ++i)
            f->C[i] = SIMD_MM(add_ps)(f->C[i], f->dC[i]);
    }
    else
    {
        for (int i = 0; i <= Coeff::m2; ++i)
            f->C[i] = SIMD_MM(add_ps)(f->C[i], f->dC[i]);
    }

    auto v3 = SUB(vin, f->R[Reg::ic2eq]);

    // v1 = a1 * ic1eq + a2 * v3
    auto v1 = ADD(MUL(f->C[Coeff::a1], f->R[Reg::ic1eq]), MUL(f->C[Coeff::a2], v3));

    // v2 = ic2eq + a2 * ic1eq + a3 * v3
    auto v2 = ADD(f->R[Reg::ic2eq],
                  ADD(MUL(f->C[Coeff::a2], f->R[Reg::ic1eq]), MUL(f->C[Coeff::a3], v3)));

    // ic1eq = 2 * v1 - ic1eq
    auto twoSSE = SIMD_MM(set1_ps)(2.0f);
    f->R[Reg::ic1eq] = SUB(MUL(twoSSE, v1), f->R[Reg::ic1eq]);

    // ic2eq = 2 * v2 - ic2eq
    f->R[Reg::ic2eq] = SUB(MUL(twoSSE, v2), f->R[Reg::ic2eq]);

    // (m0 * input) + ((m1 * v1) + (m2 * v2))
    if (lowPassOpt) // m0 == m1 = 0
        return v2;
    else
        return ADD(MUL(f->C[Coeff::m0], vin),
                   ADD(MUL(f->C[Coeff::m1], v1), MUL(f->C[Coeff::m2], v2)));
}

#undef ADD
#undef SUB
#undef DIV
#undef MUL
#undef SETALL
} // namespace sst::filters::cytomic_quadform

#endif // CYTOMICSVFQUADFORM_H
