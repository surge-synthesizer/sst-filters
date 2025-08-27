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

#ifndef INCLUDE_SST_FILTERS_CYTOMICSVF_H
#define INCLUDE_SST_FILTERS_CYTOMICSVF_H

#include <algorithm>
#include <cmath>
#include <cassert>
#include <complex>

#include <iostream>

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
 * An implementation of "Solving the continuous SVF equations using
 * trapezoidal integration and equivalent currents" by Andy Simper
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
 *
 * To use this in the filters++ or filters api, see the
 * re-implementation in CytomicSVFQuadForm
 */

namespace sst::filters
{

#define ADD(a, b) SIMD_MM(add_ps)(a, b)
#define SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define DIV(a, b) SIMD_MM(div_ps)(a, b)
#define MUL(a, b) SIMD_MM(mul_ps)(a, b)
#define SETALL(a) SIMD_MM(set1_ps)(a)

struct CytomicSVF
{
    SIMD_M128 ic1eq{SIMD_MM(setzero_ps)()}, ic2eq{SIMD_MM(setzero_ps)()};
    SIMD_M128 g, k, gk, a1, a2, a3, m0, m1, m2;

    SIMD_M128 oneSSE{SETALL(1.0)};
    SIMD_M128 negoneSSE{SETALL(-1.0)};
    SIMD_M128 twoSSE{SETALL(2.0)};
    SIMD_M128 negtwoSSE{SETALL(-2.0)};

    enum struct Mode : uint32_t
    {
        Lowpass,
        Highpass,
        Bandpass,
        Notch,
        Peak,
        Allpass,
        Bell,
        LowShelf,
        HighShelf
    };

    /*
     * Mode per above
     * freq is frequency in Hz
     * Resonance is 0...1 resonance
     * srInv is the inverse sample rate
     * bellShelfAmp is only used in Bell/LowShelf/HighShelf for the amplitude.
     * The Cytomic documents use 10^db/40 scale, but here we assume the pow() happens
     * outside throught some other means
     */
    void setCoeff(Mode mode, float freq, float res, float srInv, float bellShelfAmp = 1.f)
    {
        auto conorm = std::clamp(freq * srInv, 0.f, 0.499f); // stable until nyquist
        res = std::clamp(res, 0.f, 0.98f);
        bellShelfAmp = std::max(bellShelfAmp, 0.001f);

        g = SETALL(sst::basic_blocks::dsp::fasttan(M_PI * conorm));
        k = SETALL(2.0 - 2 * res);

        if (mode == Mode::Bell)
        {
            k = DIV(k, SETALL(bellShelfAmp));
        }
        setCoeffPostGK(mode, SETALL(bellShelfAmp));
    }

    void setCoeff(Mode mode, float freqL, float freqR, float resL, float resR, float srInv,
                  float bellShelfAmpL, float bellShelfAmpR)
    {
        auto coL = M_PI * std::clamp(freqL * srInv, 0.f, 0.499f); // stable until Nyquist
        auto coR = M_PI * std::clamp(freqR * srInv, 0.f, 0.499f); // stable until Nyquist
        g = sst::basic_blocks::dsp::fasttanSSE(SIMD_MM(set_ps)(0, 0, coR, coL));
        auto res =
            SIMD_MM(set_ps)(0, 0, std::clamp(resR, 0.f, 0.98f), std::clamp(resL, 0.f, 0.98f));

        auto bellShelfAmp =
            SIMD_MM(set_ps)(0, 0, std::max(bellShelfAmpL, 0.001f), std::max(bellShelfAmpR, 0.001f));

        k = SUB(twoSSE, MUL(twoSSE, res));
        if (mode == Mode::Bell)
        {
            k = DIV(k, bellShelfAmp);
        }
        setCoeffPostGK(mode, bellShelfAmp);
    }

    void setCoeffPostGK(Mode mode, SIMD_M128 bellShelfSSE)
    {
        gk = ADD(g, k);
        a1 = DIV(oneSSE, ADD(oneSSE, MUL(g, gk)));
        a2 = MUL(g, a1);
        a3 = MUL(g, a2);

        switch (mode)
        {
        case Mode::Lowpass:
            m0 = SIMD_MM(setzero_ps)();
            m1 = SIMD_MM(setzero_ps)();
            m2 = oneSSE;
            break;
        case Mode::Bandpass:
            m0 = SIMD_MM(setzero_ps)();
            m1 = oneSSE;
            m2 = SIMD_MM(setzero_ps)();
            break;
        case Mode::Highpass:
            m0 = oneSSE;
            m1 = SUB(SIMD_MM(setzero_ps)(), k);
            m2 = negoneSSE;
            break;
        case Mode::Notch:
            m0 = oneSSE;
            m1 = SUB(SIMD_MM(setzero_ps)(), k);
            m2 = SIMD_MM(setzero_ps)();
            break;
        case Mode::Peak:
            m0 = oneSSE;
            m1 = SUB(SIMD_MM(setzero_ps)(), k);
            m2 = negtwoSSE;
            break;
        case Mode::Allpass:
            m0 = oneSSE;
            m1 = MUL(negtwoSSE, k);
            m2 = SIMD_MM(setzero_ps)();
            break;
        case Mode::Bell:
        {
            auto A = bellShelfSSE;
            m0 = oneSSE;
            m1 = MUL(k, SUB(MUL(A, A), oneSSE));
            m2 = SIMD_MM(setzero_ps)();
        }
        break;
        case Mode::LowShelf:
        {
            auto A = bellShelfSSE;
            m0 = oneSSE;
            m1 = MUL(k, SUB(A, oneSSE));
            m2 = SUB(MUL(A, A), oneSSE);
        }
        break;
        case Mode::HighShelf:
        {
            auto A = bellShelfSSE;
            m0 = MUL(A, A);
            m1 = MUL(MUL(k, SUB(oneSSE, A)), A);
            m2 = SUB(oneSSE, MUL(A, A));
        }
        break;
        default:
            m0 = SIMD_MM(setzero_ps)();
            m1 = SIMD_MM(setzero_ps)();
            m2 = SIMD_MM(setzero_ps)();
            break;
        }
    }

    void fetchCoeffs(const CytomicSVF &that)
    {
        g = that.g;
        k = that.k;
        gk = that.gk;
        a1 = that.a1;
        a2 = that.a2;
        a3 = that.a3;
        da1 = that.da1;
        da2 = that.da2;
        da3 = that.da3;
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
        auto v3 = SUB(vin, that.ic2eq);

        // v1 = a1 * ic1eq + a2 * v3
        auto v1 = ADD(MUL(that.a1, that.ic1eq), MUL(that.a2, v3));

        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto v2 = ADD(that.ic2eq, ADD(MUL(that.a2, that.ic1eq), MUL(that.a3, v3)));

        // ic1eq = 2 * v1 - ic1eq
        that.ic1eq = SUB(MUL(that.twoSSE, v1), that.ic1eq);

        // ic2eq = 2 * v2 - ic2eq
        that.ic2eq = SUB(MUL(that.twoSSE, v2), that.ic2eq);

        // (m0 * input) + ((m1 * v1) + (m2 * v2))
        return ADD(MUL(that.m0, vin), ADD(MUL(that.m1, v1), MUL(that.m2, v2)));
    }

    /*
     * Process across a block with smoothing
     */
    SIMD_M128 da1, da2, da3;
    SIMD_M128 dm0, dm1, dm2;
    bool firstBlock{true};

    template <int blockSize>
    void setCoeffForBlock(Mode mode, float freq, float res, float srInv, float bellShelfAmp = 1.f)
    {
        // Preserve the prior values
        SIMD_M128 a1_prior = a1;
        SIMD_M128 a2_prior = a2;
        SIMD_M128 a3_prior = a3;

        SIMD_M128 m0_prior = m0;
        SIMD_M128 m1_prior = m1;
        SIMD_M128 m2_prior = m2;

        // calculate the new values
        setCoeff(mode, freq, res, srInv, bellShelfAmp);

        // If its the first time around snap them
        if (firstBlock)
        {
            a1_prior = a1;
            a2_prior = a2;
            a3_prior = a3;
            m0_prior = m0;
            m1_prior = m1;
            m2_prior = m2;
            firstBlock = false;
        }

        // then for each one calculate the change across the block
        static constexpr float obsf = 1.f / blockSize;
        auto obs = SETALL(obsf);

        // and set the changeup, and reset as to the prior value so we move in the block
        da1 = MUL(SUB(a1, a1_prior), obs);
        a1 = a1_prior;

        da2 = MUL(SUB(a2, a2_prior), obs);
        a2 = a2_prior;

        da3 = MUL(SUB(a3, a3_prior), obs);
        a3 = a3_prior;

        dm0 = MUL(SUB(m0, m0_prior), obs);
        m0 = m0_prior;

        dm1 = MUL(SUB(m1, m1_prior), obs);
        m1 = m1_prior;

        dm2 = MUL(SUB(m2, m2_prior), obs);
        m2 = m2_prior;
    }

    // it's a bit annoying this is a copy but I am sure a clever future me will do better
    template <int blockSize>
    void setCoeffForBlock(Mode mode, float freqL, float freqR, float resL, float resR, float srInv,
                          float bellShelfAmpL, float bellShelfAmpR)
    {
        SIMD_M128 a1_prior = a1;
        SIMD_M128 a2_prior = a2;
        SIMD_M128 a3_prior = a3;

        SIMD_M128 m0_prior = m0;
        SIMD_M128 m1_prior = m1;
        SIMD_M128 m2_prior = m2;

        setCoeff(mode, freqL, freqR, resL, resR, srInv, bellShelfAmpL, bellShelfAmpR);

        if (firstBlock)
        {
            a1_prior = a1;
            a2_prior = a2;
            a3_prior = a3;
            m0_prior = m0;
            m1_prior = m1;
            m2_prior = m2;
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

        dm0 = MUL(SUB(m0, m0_prior), obs);
        m0 = m0_prior;

        dm1 = MUL(SUB(m1, m1_prior), obs);
        m1 = m1_prior;

        dm2 = MUL(SUB(m2, m2_prior), obs);
        m2 = m2_prior;
    }

    template <int blockSize> void retainCoeffForBlock()
    {
        da1 = SIMD_MM(setzero_ps)();
        da2 = SIMD_MM(setzero_ps)();
        da3 = SIMD_MM(setzero_ps)();
        dm0 = SIMD_MM(setzero_ps)();
        dm1 = SIMD_MM(setzero_ps)();
        dm2 = SIMD_MM(setzero_ps)();
    }

    void processBlockStep(float &L, float &R)
    {
        step(*this, L, R);
        a1 = ADD(a1, da1);
        a2 = ADD(a2, da2);
        a3 = ADD(a3, da3);
        m1 = ADD(m1, dm1);
        m2 = ADD(m2, dm2);
        m0 = ADD(m0, dm0);
    }

    void processBlockStep(float &L)
    {
        float tmp{0.f};
        step(*this, L, tmp);
        a1 = ADD(a1, da1);
        a2 = ADD(a2, da2);
        a3 = ADD(a3, da3);
        m1 = ADD(m1, dm1);
        m2 = ADD(m2, dm2);
        m0 = ADD(m0, dm0);
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

#undef ADD
#undef SUB
#undef DIV
#undef MUL
#undef SETALL
};

/**
 * CytomicSVFGainAt returns the linear amplitude gain for a given mode cutoff res amp at a given
 * frequency. It is based on the complex analytic formulas in the above referenced PDF at page 12
 * (for the non-shelf-bell ones) and beyond (for bell shelf)
 *
 * @param mode
 * @param cutoff
 * @param res
 * @param bellShelfAmp
 * @param atFrequency
 * @return
 */
inline float CytomicSVFGainAt(CytomicSVF::Mode mode, float cutoff, float res, float bellShelfAmp,
                              float atFrequency)
{
    double srInv = 1.0 / 48000;

    auto c = cutoff * srInv;

    double g = tan(M_PI * c);
    std::complex<double> ic(0, 1);
    auto z = exp(2.0 * M_PI * ic * (double)atFrequency * srInv);
    auto k = 2 - 2 * res;

    // read as "z plus 1 squared", "z minus one quared" and "minus one plus z squared"
    auto zp1_sqrd = (1. + z) * (1. + z);
    auto zm1_sqrd = (-1. + z) * (-1. + z);
    auto m1p_zsqrd = (-1. + z * z);
    std::complex<double> resC;

    switch (mode)
    {
    case CytomicSVF::Mode::Lowpass:
    {
        auto num = g * g * zp1_sqrd;
        auto den = zm1_sqrd + g * g * zp1_sqrd + g * k * m1p_zsqrd;
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::Bandpass:
    {
        auto num = g * m1p_zsqrd;
        auto den = zm1_sqrd + g * g * zp1_sqrd + g * k * m1p_zsqrd;
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::Highpass:
    {
        auto num = zm1_sqrd;
        auto den = zm1_sqrd + g * g * zp1_sqrd + g * k * m1p_zsqrd;
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::Notch:
    {
        auto num = zm1_sqrd + g * g * zp1_sqrd;
        auto den = zm1_sqrd + g * g * zp1_sqrd + g * k * m1p_zsqrd;
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::Peak:
    {
        auto n1 = (1.0 + g + (g - 1.0) * z);
        auto n2 = (-1.0 + g + z + g * z);
        auto num = n1 * n2;
        auto den = zm1_sqrd + g * g * zp1_sqrd + g * k * m1p_zsqrd;
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::Allpass:
    {
        resC = {1.0, 0.0};
    }
    break;
    case CytomicSVF::Mode::Bell:
    {
        double A = bellShelfAmp;
        auto m0 = 1.0;
        auto m1 = (-1 + A * A) * k / A;
        auto num = g * k * m0 * (-1. + z * z) +
                   A * (g * (1. + z) * (m1 * (-1. + z)) + m0 * (zm1_sqrd + g * g * zp1_sqrd));
        auto den = g * k * m1p_zsqrd + A * (zm1_sqrd + g * g * zp1_sqrd);
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::LowShelf:
    {
        double A = bellShelfAmp;

        auto m0 = 1.0;
        auto m1 = (-1 + A) * k;
        auto m2 = -1 + A * A;
        auto num = A * m0 * zm1_sqrd + g * g * (m0 + m2) * zp1_sqrd +
                   std::sqrt(A) * g * (k * m0 + m1) * m1p_zsqrd;
        auto den = A * zm1_sqrd + g * g * zp1_sqrd + std::sqrt(A) * g * k * m1p_zsqrd;
        resC = num / den;
    }
    break;
    case CytomicSVF::Mode::HighShelf:
    {
        double A = bellShelfAmp;
        auto sqrtA = std::sqrt(A);

        auto m0 = A * A;
        auto m1 = -(-1 + A) * A * k;
        auto m2 = 1 - A * A;

        auto np1 = sqrtA * g * (1. + z);
        auto np2 = m1 * (-1. + z) + sqrtA * g * m2 * (1. + z);
        auto np3 = zm1_sqrd + A * g * g * zp1_sqrd + sqrtA * g * k * m1p_zsqrd;
        auto num = np1 * np2 + m0 * np3;
        auto den = zm1_sqrd + A * g * g * zp1_sqrd + sqrtA * g * k * m1p_zsqrd;

        resC = num / den;
    }
    break;
    }

    return std::abs(resC);
}

/**
 * This is a convenience function to give you an entire curve worth over a reasonable frequency
 * bound with the frequencies log spaced. You know, just feed this to a juce::Path and paint type
 * thing, probably with a log on the gain result.
 *
 * @param mode
 * @param cutoff
 * @param res
 * @param bellShelfAmp
 * @param freqOut
 * @param gainOut
 * @param N
 */
inline void CytomicSVFGainProfile(CytomicSVF::Mode mode, float cutoff, float res,
                                  float bellShelfAmp, float *freqOut, float *gainOut, size_t N)
{
    auto loFreq = 10.f;
    auto hiFreq = 15000;
    auto dFreq = (hiFreq - loFreq);

    for (int i = 0; i < N; ++i)
    {
        auto n2 = std::pow(2.0, i * 1.0 / (N - 1));
        auto freq = loFreq + (n2 - 1) * dFreq;

        freqOut[i] = freq;
        gainOut[i] = CytomicSVFGainAt(mode, cutoff, res, bellShelfAmp, freq);
    }
}
} // namespace sst::filters

#endif // SHORTCIRCUITXT_CYTOMICSVF_H
