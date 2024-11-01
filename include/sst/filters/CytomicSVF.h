/*
 * sst-filters - A header-only collection of SIMD filter
 * implementations by the Surge Synth Team
 *
 * Copyright 2019-2024, various authors, as described in the GitHub
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
 * An important noce on Licensing. sst-filters is, by and large,
 * gpl3 code, but Andy kindly made his entire work on this filter
 * available to everyone, so this header file s available
 * for you to copy in an MIT licensed context. You will have to
 * implement your own fast tan sse or use tanh below, but basically
 * go anead and use it however you want, in the same spirit as andy's
 * sharing the work.
 */

#ifndef INCLUDE_SST_FILTERS_CYTOMICSVF_H
#define INCLUDE_SST_FILTERS_CYTOMICSVF_H

#include <algorithm>
#include <cmath>
#include <cassert>

#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) ||                                   \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#else
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#endif

#include "sst/basic-blocks/dsp/FastMath.h"

/*
 * An implementation of "Solving the continuous SVF equations using
 * trapezoidal integration and equivalent currents" by Andy Simpler
 * @ cytomic
 *
 * https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
 *
 * structured so you can do SSE-based parallel stereo and
 * with functions for each mode allowing minimal overhead
 *
 * Higher level functions like when to call the setCoeff
 * and how to run as a block are left as an exercise to the client of this
 * class. This is just the raw functions coded up.
 */

namespace sst::filters
{
struct CytomicSVF
{
    SIMD_M128 ic1eq{SIMD_MM(setzero_ps)()}, ic2eq{SIMD_MM(setzero_ps)()};
    SIMD_M128 g, k, gk, a1, a2, a3, m0, m1, m2;

    SIMD_M128 oneSSE{SIMD_MM(set1_ps)(1.0)};
    SIMD_M128 negoneSSE{SIMD_MM(set1_ps)(-1.0)};
    SIMD_M128 twoSSE{SIMD_MM(set1_ps)(2.0)};
    SIMD_M128 negtwoSSE{SIMD_MM(set1_ps)(-2.0)};
    enum Mode
    {
        LP,
        HP,
        BP,
        NOTCH,
        PEAK,
        ALL,
        BELL,
        LOW_SHELF,
        HIGH_SHELF
    };

    /*
     * Mode per above
     * freq is frequency in hz
     * Resonance is 0...1 resonance
     * srInv is the inverse sample rate
     * bellShelfAmp is only used in BELL/LOW_SHELF/HIGH_SHELF for the amplitude.
     * The cytomic documents use 10^db/40 scale but here we assume the pow happens
     * outside throught some other means
     */
    void setCoeff(Mode mode, float freq, float res, float srInv, float bellShelfAmp = 1.f)
    {
        auto conorm = std::clamp(freq * srInv, 0.f, 0.499f); // stable until nyquist
        res = std::clamp(res, 0.f, 0.98f);
        bellShelfAmp = std::max(bellShelfAmp, 0.001f);

        g = SIMD_MM(set1_ps)(sst::basic_blocks::dsp::fasttan(M_PI * conorm));
        k = SIMD_MM(set1_ps)(2.0 - 2 * res);
        if (mode == BELL)
        {
            k = SIMD_MM(div_ps)(k, SIMD_MM(set1_ps)(bellShelfAmp));
        }
        setCoeffPostGK(mode, SIMD_MM(set1_ps)(bellShelfAmp));
    }

    void setCoeff(Mode mode, float freqL, float freqR, float resL, float resR, float srInv,
                  float bellShelfAmpL, float bellShelfAmpR)
    {
        auto coL = M_PI * std::clamp(freqL * srInv, 0.f, 0.499f); // stable until nyquist
        auto coR = M_PI * std::clamp(freqR * srInv, 0.f, 0.499f); // stable until nyquist
        g = sst::basic_blocks::dsp::fasttanhSSE(SIMD_MM(set_ps)(0, 0, coR, coL));
        auto res =
            SIMD_MM(set_ps)(0, 0, std::clamp(resR, 0.f, 0.98f), std::clamp(resL, 0.f, 0.98f));

        auto bellShelfAmp =
            SIMD_MM(set_ps)(0, 0, std::max(bellShelfAmpL, 0.001f), std::max(bellShelfAmpR, 0.001f));

        k = SIMD_MM(sub_ps)(twoSSE, SIMD_MM(mul_ps)(twoSSE, res));
        if (mode == BELL)
        {
            k = SIMD_MM(div_ps)(k, bellShelfAmp);
        }
        setCoeffPostGK(mode, bellShelfAmp);
    }

    void setCoeffPostGK(Mode mode, SIMD_M128 bellShelfSSE)
    {
        gk = SIMD_MM(add_ps)(g, k);
        a1 = SIMD_MM(div_ps)(oneSSE, SIMD_MM(add_ps)(oneSSE, SIMD_MM(mul_ps)(g, gk)));
        a2 = SIMD_MM(mul_ps)(g, a1);
        a3 = SIMD_MM(mul_ps)(g, a2);

        switch (mode)
        {
        case LP:
            m0 = SIMD_MM(setzero_ps)();
            m1 = SIMD_MM(setzero_ps)();
            m2 = oneSSE;
            break;
        case BP:
            m0 = SIMD_MM(setzero_ps)();
            m1 = oneSSE;
            m2 = SIMD_MM(setzero_ps)();
            break;
        case HP:
            m0 = oneSSE;
            m1 = SIMD_MM(sub_ps)(SIMD_MM(setzero_ps)(), k);
            m2 = negoneSSE;
            break;
        case NOTCH:
            m0 = oneSSE;
            m1 = SIMD_MM(sub_ps)(SIMD_MM(setzero_ps)(), k);
            m2 = SIMD_MM(setzero_ps)();
            break;
        case PEAK:
            m0 = oneSSE;
            m1 = SIMD_MM(sub_ps)(SIMD_MM(setzero_ps)(), k);
            m2 = negtwoSSE;
            break;
        case ALL:
            m0 = oneSSE;
            m1 = SIMD_MM(mul_ps)(negtwoSSE, k);
            m2 = SIMD_MM(setzero_ps)();
            break;
        case BELL:
        {
            auto A = bellShelfSSE;
            m0 = oneSSE;
            m1 = SIMD_MM(mul_ps)(k, SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(A, A), oneSSE));
            m2 = SIMD_MM(setzero_ps)();
        }
        break;
        case LOW_SHELF:
        {
            auto A = bellShelfSSE;
            m0 = oneSSE;
            m1 = SIMD_MM(mul_ps)(k, SIMD_MM(sub_ps)(A, oneSSE));
            m2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(A, A), oneSSE);
        }
        break;
        case HIGH_SHELF:
        {
            auto A = bellShelfSSE;
            m0 = SIMD_MM(mul_ps)(A, A);
            m1 = SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(k, SIMD_MM(sub_ps)(oneSSE, A)), A);
            m2 = SIMD_MM(sub_ps)(oneSSE, SIMD_MM(mul_ps)(A, A));
        }
        break;
        default:
            m0 = SIMD_MM(setzero_ps)();
            m1 = SIMD_MM(setzero_ps)();
            m2 = SIMD_MM(setzero_ps)();
            break;
        }
    }

    void fetchCoeffs(const CytomicSVF &that) // TODO: Block smoothed version
    {
        g = that.g;
        k = that.k;
        gk = that.gk;
        a1 = that.a1;
        a2 = that.a2;
        a3 = that.a3;
        m0 = that.m0;
        m1 = that.m1;
        m2 = that.m2;
    }

    static void step(CytomicSVF &that, float &L, float &R)
    {
        auto vin = SIMD_MM(set_ps)(0, 0, R, L);
        auto res = stepSSE(that, vin);
        float r4 alignas(16)[4];
        SIMD_MM(store_ps)(r4, res);
        L = r4[0];
        R = r4[1];
    }

    static SIMD_M128 stepSSE(CytomicSVF &that, SIMD_M128 vin)
    {
        // v3 = v0 - ic2eq
        auto v3 = SIMD_MM(sub_ps)(vin, that.ic2eq);

        // v1 = a1 * ic1eq + a2 * v3
        auto v1 =
            SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.a1, that.ic1eq), SIMD_MM(mul_ps)(that.a2, v3));

        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto v2 = SIMD_MM(add_ps)(that.ic2eq, SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.a2, that.ic1eq),
                                                              SIMD_MM(mul_ps)(that.a3, v3)));

        // ic1eq = 2 * v1 - ic1eq
        that.ic1eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, v1), that.ic1eq);

        // ic2eq = 2 * v2 - ic2eq
        that.ic2eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, v2), that.ic2eq);

        return SIMD_MM(add_ps)(
            SIMD_MM(mul_ps)(that.m0, vin),
            SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.m1, v1), SIMD_MM(mul_ps)(that.m2, v2)));
    }

    /*
     * Process across a block with smoothing
     */
    SIMD_M128 a1_prior, a2_prior, a3_prior;
    SIMD_M128 da1, da2, da3;
    bool firstBlock{true};

    template <int blockSize>
    void setCoeffForBlock(Mode mode, float freq, float res, float srInv, float bellShelfAmp = 1.f)
    {
        // Preserve the prior values
        a1_prior = a1;
        a2_prior = a2;
        a3_prior = a3;

        // calculate the new a1s
        setCoeff(mode, freq, res, srInv, bellShelfAmp);

        // If its the first time around snap them
        if (firstBlock)
        {
            a1_prior = a1;
            a2_prior = a2;
            a3_prior = a3;
            firstBlock = false;
        }

        // then for each one calculate the change across the block
        static constexpr float obsf = 1.f / blockSize;
        auto obs = SIMD_MM(set1_ps)(obsf);

        // and set the changeup, and reset a1 to the prior value so we move in the block
        da1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a1, a1_prior), obs);
        a1 = a1_prior;

        da2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a2, a2_prior), obs);
        a2 = a2_prior;

        da3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a3, a3_prior), obs);
        a3 = a3_prior;
    }

    // it's a bit annoying this is a copy but I am sure a clever future me will do better
    template <int blockSize>
    void setCoeffForBlock(Mode mode, float freqL, float freqR, float resL, float resR, float srInv,
                          float bellShelfAmpL, float bellShelfAmpR)
    {
        // Preserve the prior values
        a1_prior = a1;
        a2_prior = a2;
        a3_prior = a3;

        // calculate the new a1s
        setCoeff(mode, freqL, freqR, resL, resR, srInv, bellShelfAmpL, bellShelfAmpR);

        // If its the first time around snap them
        if (firstBlock)
        {
            a1_prior = a1;
            a2_prior = a2;
            a3_prior = a3;
            firstBlock = false;
        }

        // then for each one calculate the change across the block
        static constexpr float obsf = 1.f / blockSize;
        auto obs = SIMD_MM(set1_ps)(obsf);

        // and set the changeup, and reset a1 to the prior value so we move in the block
        da1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a1, a1_prior), obs);
        a1 = a1_prior;

        da2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a2, a2_prior), obs);
        a2 = a2_prior;

        da3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a3, a3_prior), obs);
        a3 = a3_prior;
    }

    template <int blockSize> void retainCoeffForBlock()
    {
        da1 = SIMD_MM(setzero_ps)();
        da2 = SIMD_MM(setzero_ps)();
        da3 = SIMD_MM(setzero_ps)();
    }

    void processBlockStep(float &L, float &R)
    {
        step(*this, L, R);
        a1 = SIMD_MM(add_ps)(a1, da1);
        a2 = SIMD_MM(add_ps)(a2, da2);
        a3 = SIMD_MM(add_ps)(a3, da3);
    }

    void processBlockStep(float &L)
    {
        float tmp{0.f};
        step(*this, L, tmp);
        a1 = SIMD_MM(add_ps)(a1, da1);
        a2 = SIMD_MM(add_ps)(a2, da2);
        a3 = SIMD_MM(add_ps)(a3, da3);
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
        ic1eq = SIMD_MM(setzero_ps)();
        ic2eq = SIMD_MM(setzero_ps)();
    }
};
} // namespace sst::filters

#endif // SHORTCIRCUITXT_CYTOMICSVF_H
