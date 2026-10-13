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

/*
 * An important note on licensing. sst-filters is, by and large,
 * GPL3 code, but this filter builds on Andy Simper's SVF, which he kindly
 * made available to everyone, so this header file is available
 * for you to copy in an MIT licensed context. You will have to
 * implement your own FastTan and FastTanh SSE, but basically
 * go ahead and use it however you want, in the same spirit as Andy's
 * sharing the work.
 */

#ifndef INCLUDE_SST_FILTERS_DETAIL_CYTOMICSVFCASCADEDETAILS_H
#define INCLUDE_SST_FILTERS_DETAIL_CYTOMICSVFCASCADEDETAILS_H

#include <algorithm>
#include <cmath>
#include <cstdint>

#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) ||                                   \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#else
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#endif

#include "sst/basic-blocks/dsp/FastMath.h"

/*
 * The coefficient layout and per sample kernel behind CytomicSVFCascade and
 * its QuadFilterUnit form. Not public api; include CytomicSVFCascade.h.
 */

namespace sst::filters::cytomic_cascade
{
enum struct Mode : uint32_t
{
    Lowpass,
    Highpass,
    Bandpass,
    Notch,
    Peak,
    Allpass
};

namespace detail
{
// two critically damped cores, 1/(1+s)^4, hit -180 at s = j with loop gain 1/4
static constexpr float rMax = 4.f;

// resonance 0..1 maps to this fraction of self-oscillation
static constexpr double drivenResonanceMax = 1.25;
static constexpr double linearResonanceMax = 0.99;

/*
 * Only q = g / (1+g) ramps; the core coefficients follow from it each sample so
 * every sample is an exact critically damped core, which the mixes rely on.
 */
struct Coeff
{
    // not an enum to avoid annoying casts
    static constexpr int q = 0;
    static constexpr int r = 1;
    static constexpr int inv = 2;
    static constexpr int gain = 3;
    static constexpr int count = 4;
};

// freq in Hz is the resonant peak; res is 0..1
template <bool Driven> inline void makeCoeffs(double freq, double res, double srInv, float *out)
{
    auto g = std::tan(M_PI * std::clamp(freq * srInv, 5.0 * srInv, 0.49));
    auto r = rMax * (Driven ? std::clamp(res, 0.0, 1.0) * drivenResonanceMax
                            : std::clamp(res, 0.0, linearResonanceMax));
    auto q = g / (1.0 + g);
    auto q4 = q * q * q * q;

    out[Coeff::q] = (float)q;
    out[Coeff::r] = (float)r;
    out[Coeff::inv] = (float)(1.0 / (1.0 + r * q4));
    out[Coeff::gain] = (float)(1.0 + r);
}

#define ADD(a, b) SIMD_MM(add_ps)(a, b)
#define SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define DIV(a, b) SIMD_MM(div_ps)(a, b)
#define MUL(a, b) SIMD_MM(mul_ps)(a, b)
#define SETALL(a) SIMD_MM(set1_ps)(a)

inline void advanceCoeffs(SIMD_M128 *__restrict C, const SIMD_M128 *__restrict dC)
{
    for (int i = 0; i < Coeff::count; ++i)
        C[i] = ADD(C[i], dC[i]);
}

/*
 * One sample. ic1 and ic2 are two registers each. Coefficients are not
 * advanced here; see advanceCoeffs.
 */
template <Mode M, bool Driven, bool Compensated>
inline SIMD_M128 step(const SIMD_M128 *__restrict C, SIMD_M128 *__restrict ic1,
                      SIMD_M128 *__restrict ic2, SIMD_M128 x)
{
    // with k = 2, a1 = 1/(1+g)^2, a2 = g a1 and a3 = g a2
    const auto one = SETALL(1.f);
    const auto q = C[Coeff::q];
    const auto p = SUB(one, q);
    const auto a1 = MUL(p, p);
    const auto a2 = MUL(p, q);
    const auto a3 = MUL(q, q);
    const auto b = SUB(one, a3);
    const auto r = C[Coeff::r];

    // driven compensates ahead of the tanh so the gain cannot lift self-oscillation
    if constexpr (Compensated && Driven)
        x = MUL(x, C[Coeff::gain]);

    // each core is v2 = a3 * in + S with S from state, so the loop solves in closed form
    auto Sa = ADD(MUL(a2, ic1[0]), MUL(b, ic2[0]));
    auto Sb = ADD(MUL(a2, ic1[1]), MUL(b, ic2[1]));
    auto St = ADD(MUL(a3, Sa), Sb);
    auto u = MUL(SUB(x, MUL(r, St)), C[Coeff::inv]);

    if constexpr (Driven)
    {
        // solve u + r tanh(a3^2 u + St) = x by newton, seeded with the linear solution
        const auto gt = MUL(a3, a3);
        const auto rgt = MUL(r, gt);
        for (int it = 0; it < 2; ++it)
        {
            auto th = sst::basic_blocks::dsp::fasttanhSSEclamped(ADD(MUL(gt, u), St));
            auto f = SUB(ADD(u, MUL(r, th)), x);
            auto fp = ADD(one, MUL(rgt, SUB(one, MUL(th, th))));
            u = SUB(u, DIV(f, fp));
        }
    }

    auto v1a = ADD(MUL(a1, ic1[0]), MUL(a2, SUB(u, ic2[0])));
    auto v2a = ADD(MUL(a3, u), Sa);
    auto v1b = ADD(MUL(a1, ic1[1]), MUL(a2, SUB(v2a, ic2[1])));
    auto v2b = ADD(MUL(a3, v2a), Sb);
    ic1[0] = SUB(ADD(v1a, v1a), ic1[0]);
    ic2[0] = SUB(ADD(v2a, v2a), ic2[0]);
    ic1[1] = SUB(ADD(v1b, v1b), ic1[1]);
    ic2[1] = SUB(ADD(v2b, v2b), ic2[1]);

    // v2b is u/(1+s)^4, v1a is s u/(1+s)^2 and hp2 is s^2 u/(1+s)^4; D = (1+s)^4 + r
    SIMD_M128 out;
    if constexpr (M == Mode::Lowpass)
    {
        out = v2b;
    }
    else
    {
        auto hp2 = SUB(SUB(v2a, ADD(v1b, v1b)), v2b);
        if constexpr (M == Mode::Highpass)
        {
            // s^4 / D
            out = ADD(SUB(SUB(u, MUL(SETALL(4.f), v1a)), v2b), ADD(hp2, hp2));
        }
        else if constexpr (M == Mode::Bandpass)
        {
            // 4 s^2 / D, unity peak at zero resonance
            out = MUL(SETALL(4.f), hp2);
        }
        else if constexpr (M == Mode::Allpass)
        {
            // ((1-s)^4 + r) / D, with x - u supplying the r
            out = ADD(SUB(x, MUL(SETALL(8.f), v1a)), MUL(SETALL(16.f), hp2));
        }
        else
        {
            // zero pair at s = j; past rMax in the driven loop the notch just closes
            auto c = SUB(SETALL(rMax), r);
            if constexpr (Driven)
                c = SIMD_MM(max_ps)(c, SIMD_MM(setzero_ps)());
            auto z = ADD(MUL(SETALL(0.5f), hp2), MUL(SETALL(0.25f), v1a));
            out = SUB(x, MUL(c, z));
            if constexpr (M == Mode::Peak)
            {
                // notch less twice the unity gain lowpass, as in the 12dB svf
                auto g2 = ADD(C[Coeff::gain], C[Coeff::gain]);
                out = SUB(out, MUL(g2, v2b));
            }
        }
    }

    if constexpr (Compensated && !Driven)
        out = MUL(out, C[Coeff::gain]);

    return out;
}

// per sample q and inv for audio rate cutoff, using the current r
inline void rebuildCoeffsFromFreq(SIMD_M128 *C, SIMD_M128 freq, float srInv)
{
    const auto one = SETALL(1.f);
    auto fn = SIMD_MM(min_ps)(SIMD_MM(max_ps)(MUL(freq, SETALL(srInv)), SETALL(5.f * srInv)),
                              SETALL(0.49f));
    auto g = sst::basic_blocks::dsp::fasttanSSE(MUL(fn, SETALL((float)M_PI)));
    auto q = DIV(g, ADD(one, g));
    auto q2 = MUL(q, q);
    C[Coeff::q] = q;
    C[Coeff::inv] = DIV(one, ADD(one, MUL(C[Coeff::r], MUL(q2, q2))));
}

#undef ADD
#undef SUB
#undef DIV
#undef MUL
#undef SETALL
} // namespace detail
} // namespace sst::filters::cytomic_cascade

#endif // INCLUDE_SST_FILTERS_DETAIL_CYTOMICSVFCASCADEDETAILS_H
