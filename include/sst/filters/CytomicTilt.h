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
 * GPL3 code, but Andy kindly made his entire work on this filter
 * available to everyone, so this header file is available
 * for you to copy in an MIT licensed context. You will have to
 * implement your own FastTan SSE or use tanh below, but basically
 * go ahead and use it however you want, in the same spirit as Andy's
 * sharing the work.
 */

#ifndef INCLUDE_SST_FILTERS_CYTOMICTILT_H
#define INCLUDE_SST_FILTERS_CYTOMICTILT_H

#include <algorithm>

#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) ||                                   \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#else
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#endif

#include "sst/basic-blocks/dsp/FastMath.h"

#include <complex>

/*
 * This is just the Cytomic SVF again,
 * but two of them configured as
 * low/high shelves to act as a tilt EQ
 * with most of the coefficients between them
 */

namespace sst::filters
{

#define ADD(a, b) SIMD_MM(add_ps)(a, b)
#define SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define DIV(a, b) SIMD_MM(div_ps)(a, b)
#define MUL(a, b) SIMD_MM(mul_ps)(a, b)
#define SETALL(a) SIMD_MM(set1_ps)(a)

struct CytomicTilt
{
    SIMD_M128 ic1eqLo{SIMD_MM(setzero_ps)()}, ic2eqLo{SIMD_MM(setzero_ps)()},
        ic1eqHi{SIMD_MM(setzero_ps)()}, ic2eqHi{SIMD_MM(setzero_ps)()};

    SIMD_M128 g, k, gk, a1, a2, a3, m0Lo, m1Lo, m2Lo, m0Hi, m1Hi, m2Hi;

    // used for block smoothing
    SIMD_M128 da1, da2, da3;
    SIMD_M128 dm0Lo, dm1Lo, dm2Lo, dm0Hi, dm1Hi, dm2Hi;
    bool firstBlock{true};

    SIMD_M128 oneSSE{SETALL(1.0)};
    SIMD_M128 negoneSSE{SETALL(-1.0)};
    SIMD_M128 twoSSE{SETALL(2.0)};
    SIMD_M128 negtwoSSE{SETALL(-2.0)};

    /*
     * Remember, the Cytomic documents use 10^db/40 scale,
     * but our dbToLin tables/functions assume dB/20
     * so for the last arg pass in dBToLinear(dB * .5f);
     */
    void setCoeff(float freq, float res, float srInv, float dB = 1.f)
    {
        auto conorm = std::clamp(freq * srInv, 0.f, 0.499f);
        res = std::clamp(res, 0.f, 0.98f);

        g = SETALL(sst::basic_blocks::dsp::fasttan(M_PI * conorm));
        k = SETALL(2.0 - 2 * res);
        gk = ADD(g, k);
        a1 = DIV(oneSSE, ADD(oneSSE, MUL(g, gk)));
        a2 = MUL(g, a1);
        a3 = MUL(g, a2);

        auto amp = std::pow(10, dB / 40);
        auto A = SETALL(amp);
        auto invA = DIV(oneSSE, A);

        m0Lo = oneSSE;
        m1Lo = MUL(k, SUB(invA, oneSSE));
        m2Lo = SUB(MUL(invA, invA), oneSSE);

        m0Hi = MUL(A, A);
        m1Hi = MUL(MUL(k, SUB(oneSSE, A)), A);
        m2Hi = SUB(oneSSE, m0Hi);
    }

    static void step(CytomicTilt &that, float &L, float &R)
    {
        auto vin = SIMD_MM(set_ps)(0, 0, R, L);
        auto res = stepSSE(that, vin);
        float r4 alignas(16)[4];
        SIMD_MM(store_ps)(r4, res);
        L = r4[0];
        R = r4[1];
    }

    static SIMD_M128 stepSSE(CytomicTilt &that, SIMD_M128 vin)
    {
        // v3 = v0 - ic2eq
        // v1 = a1 * ic1eq + a2 * v3
        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        // ic1eq = 2 * v1 - ic1eq
        // ic2eq = 2 * v2 - ic2eq
        // m0 * v0 + m1 * v1 + m2 * v2
        // repeat

        auto v3 = SUB(vin, that.ic2eqLo);
        auto v1 = ADD(MUL(that.a1, that.ic1eqLo), MUL(that.a2, v3));
        auto v2 = ADD(that.ic2eqLo, ADD(MUL(that.a2, that.ic1eqLo), MUL(that.a3, v3)));

        that.ic1eqLo = SUB(MUL(that.twoSSE, v1), that.ic1eqLo);
        that.ic2eqLo = SUB(MUL(that.twoSSE, v2), that.ic2eqLo);

        auto lsv = ADD(MUL(that.m0Lo, vin), ADD(MUL(that.m1Lo, v1), MUL(that.m2Lo, v2)));

        auto nv3 = SUB(lsv, that.ic2eqHi);
        auto nv1 = ADD(MUL(that.a1, that.ic1eqHi), MUL(that.a2, nv3));
        auto nv2 = ADD(that.ic2eqHi, ADD(MUL(that.a2, that.ic1eqHi), MUL(that.a3, nv3)));

        that.ic1eqHi = SUB(MUL(that.twoSSE, nv1), that.ic1eqHi);
        that.ic2eqHi = SUB(MUL(that.twoSSE, nv2), that.ic2eqHi);

        return ADD(MUL(that.m0Hi, lsv), ADD(MUL(that.m1Hi, nv1), MUL(that.m2Hi, nv2)));
    }

    template <int blockSize>
    void setCoeffForBlock(float freq, float res, float srInv, float bellShelfAmp = 1.f)
    {
        SIMD_M128 a1_prior = a1;
        SIMD_M128 a2_prior = a2;
        SIMD_M128 a3_prior = a3;
        SIMD_M128 m0l_prior = m0Lo;
        SIMD_M128 m1l_prior = m1Lo;
        SIMD_M128 m2l_prior = m2Lo;
        SIMD_M128 m0h_prior = m0Hi;
        SIMD_M128 m1h_prior = m1Hi;
        SIMD_M128 m2h_prior = m2Hi;

        setCoeff(freq, res, srInv, bellShelfAmp);

        if (firstBlock)
        {
            a1_prior = a1;
            a2_prior = a2;
            a3_prior = a3;
            m0l_prior = m0Lo;
            m1l_prior = m1Lo;
            m2l_prior = m2Lo;
            m0h_prior = m0Hi;
            m1h_prior = m1Hi;
            m2h_prior = m2Hi;
            firstBlock = false;
        }

        static constexpr float obsf = 1.f / blockSize;
        auto obs = SETALL(obsf);

        da1 = MUL(SUB(a1, a1_prior), obs);
        a1 = a1_prior;

        da2 = MUL(SUB(a2, a2_prior), obs);
        a2 = a2_prior;

        da3 = MUL(SUB(a3, a3_prior), obs);
        a3 = a3_prior;

        dm0Lo = MUL(SUB(m0Lo, m0l_prior), obs);
        m0Lo = m0l_prior;

        dm1Lo = MUL(SUB(m1Lo, m1l_prior), obs);
        m1Lo = m1l_prior;

        dm2Lo = MUL(SUB(m2Lo, m2l_prior), obs);
        m2Lo = m2l_prior;

        dm0Hi = MUL(SUB(m0Hi, m0h_prior), obs);
        m0Hi = m0h_prior;

        dm1Hi = MUL(SUB(m1Hi, m1h_prior), obs);
        m1Hi = m1h_prior;

        dm2Hi = MUL(SUB(m2Hi, m2h_prior), obs);
        m2Hi = m2h_prior;
    }

    void retainCoeffForBlock()
    {
        da1 = SIMD_MM(setzero_ps)();
        da2 = SIMD_MM(setzero_ps)();
        da3 = SIMD_MM(setzero_ps)();
        dm0Lo = SIMD_MM(setzero_ps)();
        dm1Lo = SIMD_MM(setzero_ps)();
        dm2Lo = SIMD_MM(setzero_ps)();
        dm0Hi = SIMD_MM(setzero_ps)();
        dm1Hi = SIMD_MM(setzero_ps)();
        dm2Hi = SIMD_MM(setzero_ps)();
    }

    void processBlockStep(float &L, float &R)
    {
        step(*this, L, R);
        a1 = ADD(a1, da1);
        a2 = ADD(a2, da2);
        a3 = ADD(a3, da3);
        m0Lo = ADD(m0Lo, dm0Lo);
        m1Lo = ADD(m1Lo, dm1Lo);
        m2Lo = ADD(m2Lo, dm2Lo);
        m0Hi = ADD(m0Hi, dm0Hi);
        m1Hi = ADD(m1Hi, dm1Hi);
        m2Hi = ADD(m2Hi, dm2Hi);
    }

    void processBlockStep(float &L)
    {
        float tmp{0.f};
        step(*this, L, tmp);
        a1 = ADD(a1, da1);
        a2 = ADD(a2, da2);
        a3 = ADD(a3, da3);
        m0Lo = ADD(m0Lo, dm0Lo);
        m1Lo = ADD(m1Lo, dm1Lo);
        m2Lo = ADD(m2Lo, dm2Lo);
        m0Hi = ADD(m0Hi, dm0Hi);
        m1Hi = ADD(m1Hi, dm1Hi);
        m2Hi = ADD(m2Hi, dm2Hi);
    }

    template <int blockSize>
    void processBlock(const float *const inL, const float *const inR, float *outL, float *outR)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            outL[i] = inL[i];
            outR[i] = inR[i];
            processBlockStep(outL[i], outR[i]);
        }
    }

    template <int blockSize> void processBlock(const float *const inL, float *outL)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            outL[i] = inL[i];
            processBlockStep(outL[i]);
        }
    }

    void init()
    {
        ic1eqLo = SIMD_MM(setzero_ps)();
        ic2eqLo = SIMD_MM(setzero_ps)();
        ic1eqHi = SIMD_MM(setzero_ps)();
        ic2eqHi = SIMD_MM(setzero_ps)();
    }

#undef ADD
#undef SUB
#undef DIV
#undef MUL
#undef SETALL
};
} // namespace sst::filters
#endif // INCLUDE_SST_FILTERS_CYTOMICTILT_H
