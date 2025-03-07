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
#include "sst/basic-blocks/dsp/RNG.h"

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

#define ADD(a, b) SIMD_MM(add_ps)(a, b)
#define SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define DIV(a, b) SIMD_MM(div_ps)(a, b)
#define MUL(a, b) SIMD_MM(mul_ps)(a, b)
#define SETALL(a) SIMD_MM(set1_ps)(a)
#define ZERO(a) SIMD_MM(setzero_ps)(a)

struct CytomicSVF
{
    SIMD_M128 ic1eq{ZERO()}, ic2eq{ZERO()};
    SIMD_M128 g, k, gk, a1, a2, a3, m0, m1, m2;

    SIMD_M128 oneSSE{SETALL(1.0)};
    SIMD_M128 negoneSSE{SETALL(-1.0)};
    SIMD_M128 twoSSE{SETALL(2.0)};
    SIMD_M128 negtwoSSE{SETALL(-2.0)};
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

        g = SETALL(sst::basic_blocks::dsp::fasttan(M_PI * conorm));
        k = SETALL(2.0 - 2 * res);

        if (mode == BELL)
        {
            k = DIV(k, SETALL(bellShelfAmp));
        }
        setCoeffPostGK(mode, SETALL(bellShelfAmp));
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

        k = SUB(twoSSE, MUL(twoSSE, res));
        if (mode == BELL)
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
        case LP:
            m0 = ZERO();
            m1 = ZERO();
            m2 = oneSSE;
            break;
        case BP:
            m0 = ZERO();
            m1 = oneSSE;
            m2 = ZERO();
            break;
        case HP:
            m0 = oneSSE;
            m1 = SUB(ZERO(), k);
            m2 = negoneSSE;
            break;
        case NOTCH:
            m0 = oneSSE;
            m1 = SUB(ZERO(), k);
            m2 = ZERO();
            break;
        case PEAK:
            m0 = oneSSE;
            m1 = SUB(ZERO(), k);
            m2 = negtwoSSE;
            break;
        case ALL:
            m0 = oneSSE;
            m1 = MUL(negtwoSSE, k);
            m2 = ZERO();
            break;
        case BELL:
        {
            auto A = bellShelfSSE;
            m0 = oneSSE;
            m1 = MUL(k, SUB(MUL(A, A), oneSSE));
            m2 = ZERO();
        }
        break;
        case LOW_SHELF:
        {
            auto A = bellShelfSSE;
            m0 = oneSSE;
            m1 = MUL(k, SUB(A, oneSSE));
            m2 = SUB(MUL(A, A), oneSSE);
        }
        break;
        case HIGH_SHELF:
        {
            auto A = bellShelfSSE;
            m0 = MUL(A, A);
            m1 = MUL(MUL(k, SUB(oneSSE, A)), A);
            m2 = SUB(oneSSE, MUL(A, A));
        }
        break;
        default:
            m0 = ZERO();
            m1 = ZERO();
            m2 = ZERO();
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
        da1 = ZERO();
        da2 = ZERO();
        da3 = ZERO();
        dm0 = ZERO();
        dm1 = ZERO();
        dm2 = ZERO();
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
        ic1eq = ZERO();
        ic2eq = ZERO();
    }
};

template <typename hostClass> struct FastTiltNoiseFilter
{
    /*
     Ok, for our noise colors we want to feed in white noise,
     and get it back nicely filtered to an N dB/oct slope.

     11 of the above filters in series can do that really really accurately.
     But it is a bit slow, especially under modulation.

     We can improve that in two ways. Most obviously, frequencies, resonance and modes are fixed in
     this case, letting us run a bunch of clamps, mults, divs, branches plus a tan once at voice
     start which otherwise get called every time the slope changes. That change alone makes a huge
     difference under modulation.

     The other trick we're gonna do is hold 11 samples of noise in memory and run the filters in
     parallel, passing the sample train forwards to the next filter stage on each step() call,
     outputting the result of the 11th one each time. This way we can put the 11 filters
     into 3 _m128's  and do all the filtering math with SSE ops which is another big speedup.

     It also means we're 11 samples latent, but that's actually fine! The input is just a stream of
     random values anyway, so there's no phase/time relationships that we need to
     maintain. Thus we can pre-compute 11 samples on startup to get immediate output.

     We pay 11 extra samples worth of processing at the onset. But that seems a low price to pay for
     running the filters in parallel thereafter.
     */

    // initialize with a class containing dbToLinear() and getSampleRateInv() functions
    hostClass &host;

    SIMD_M128 firstQueue{ZERO()}, secondQueue{ZERO()}, thirdQueue{ZERO()};

    // 1.86 is the k value for our intended res of .07
    const SIMD_M128 kSSE = SIMD_MM(set1_ps)(1.86);
    SIMD_M128 firstG, secondG, thirdG;
    SIMD_M128 firstGK, secondGK, thirdGK;

    SIMD_M128 firstA1, secondA1, thirdA1;
    SIMD_M128 firstA2, secondA2, thirdA2;
    SIMD_M128 firstA3, secondA3, thirdA3;

    // Used in setcoeffs()
    const SIMD_M128 mask = SIMD_MM(castsi128_ps)(SIMD_MM(set_epi32)(0, 0, ~0, ~0));

    SIMD_M128 firstM0, secondM0, thirdM0;
    SIMD_M128 firstM1, secondM1, thirdM1;
    SIMD_M128 firstM2, secondM2, thirdM2;

    SIMD_M128 first1eq{ZERO()}, second1eq{ZERO()}, third1eq{ZERO()};
    SIMD_M128 first2eq{ZERO()}, second2eq{ZERO()}, third2eq{ZERO()};

    SIMD_M128 oneSSE{SETALL(1.0)};
    SIMD_M128 negoneSSE{SETALL(-1.0)};
    SIMD_M128 twoSSE{SETALL(2.0)};
    SIMD_M128 negtwoSSE{SETALL(-2.0)};

    FastTiltNoiseFilter(hostClass &hc) : host(hc) {}

    void init(float *startupNoise, float initialGain)
    {
        float G[11] = {};
        for (int i = 0; i < 11; ++i)
        {
            // TODO: what happens if sample rate is less than 44.1k? Nothing good, I bet...
            auto freq = std::powf(2, i + 1) * 10.f;

            // anyway, g = tan(freq * srInv * pi)
            G[i] = sst::basic_blocks::dsp::fasttan(M_PI * freq * host.getSampleRateInv());
        }

        // remember these are "backwards"
        firstG = SIMD_MM(set_ps)(G[3], G[2], G[1], G[0]);
        secondG = SIMD_MM(set_ps)(G[7], G[6], G[5], G[4]);
        thirdG = SIMD_MM(set_ps)(0.f, G[10], G[9], G[8]);

        // gk = g + k
        firstGK = ADD(firstG, kSSE);
        secondGK = ADD(secondG, kSSE);
        thirdGK = ADD(thirdG, kSSE);

        // a1 = 1 / (1 + g * gk)
        firstA1 = DIV(oneSSE, ADD(oneSSE, MUL(firstG, firstGK)));
        secondA1 = DIV(oneSSE, ADD(oneSSE, MUL(secondG, secondGK)));
        thirdA1 = DIV(oneSSE, ADD(oneSSE, MUL(thirdG, thirdGK)));

        // a2 = g * a1
        firstA2 = MUL(firstG, firstA1);
        secondA2 = MUL(secondG, secondA1);
        thirdA2 = MUL(thirdG, thirdA1);

        // a3 = g * a2
        firstA3 = MUL(firstG, firstA2);
        secondA3 = MUL(secondG, secondA2);
        thirdA3 = MUL(thirdG, thirdA2);

        setCoeff(initialGain);

        for (int i = 0; i < 11; ++i)
        {
            step(*this, startupNoise[i]);
        }
    }

    void setCoeff(float gain)
    {
        /*
         Six of the filters are low shelves and have their gain inverted, the rest are high shelves.
         So for each coeff start by setting the first and third m128, where every element is either
         a low or high shelf. Then the middle m128 copies from those with a blend.
         */

        float negGain = host.dbToLinear(-gain);
        float posGain = host.dbToLinear(gain);

        auto firstA = SETALL(negGain);
        auto thirdA = SETALL(posGain);
        auto secondA = SIMD_MM(blendv_ps)(thirdA, firstA, mask);

        // m0 = 1 for low shelf, A^2 for high
        firstM0 = oneSSE;
        thirdM0 = MUL(thirdA, thirdA);
        secondM0 = SIMD_MM(blendv_ps)(thirdM0, firstM0, mask);

        // m1 = k * (A - 1) for low shelf, (k * (1 - A)) * A for high
        firstM1 = MUL(kSSE, SUB(firstA, oneSSE));
        thirdM1 = MUL(MUL(kSSE, SUB(oneSSE, thirdA)), thirdA);
        secondM1 = SIMD_MM(blendv_ps)(thirdM1, firstM1, mask);

        // m2 = A^2 - 1 for low shelf, 1 - A^2 for high
        firstM2 = SUB(MUL(firstA, firstA), oneSSE);
        thirdM2 = SUB(oneSSE, MUL(thirdA, thirdA));
        secondM2 = SIMD_MM(blendv_ps)(thirdM2, firstM2, mask);
    }

    static void step(FastTiltNoiseFilter &that, float &vin)
    {
        // Is there a better way to do this shuffle? I ain't found one.
        float tQ1 alignas(16)[4];
        float tQ2 alignas(16)[4];
        float tQ3 alignas(16)[4];
        SIMD_MM(store_ps)(tQ1, that.firstQueue);
        SIMD_MM(store_ps)(tQ2, that.secondQueue);
        SIMD_MM(store_ps)(tQ3, that.thirdQueue);

        // Shuffle the queues forwards (which looks backwards), adding in the current input
        that.firstQueue = SIMD_MM(set_ps)(tQ1[2], tQ1[1], tQ1[0], vin);
        that.secondQueue = SIMD_MM(set_ps)(tQ2[2], tQ2[1], tQ2[0], tQ1[3]);
        that.thirdQueue = SIMD_MM(set_ps)(tQ3[2], tQ3[1], tQ3[0], tQ2[3]);

        // Time to run the filters!

        // v3 = input (aka v0) - ic2eq
        auto firstV3 = SUB(that.firstQueue, that.first2eq);
        auto secondV3 = SUB(that.secondQueue, that.second2eq);
        auto thirdV3 = SUB(that.thirdQueue, that.third2eq);

        // v1 = a1 * ic1eq + a2 * v3
        auto firstV1 = ADD(MUL(that.firstA1, that.first1eq), MUL(that.firstA2, firstV3));
        auto secondV1 = ADD(MUL(that.secondA1, that.second1eq), MUL(that.secondA2, secondV3));
        auto thirdV1 = ADD(MUL(that.thirdA1, that.third1eq), MUL(that.thirdA2, thirdV3));

        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto firstV2 =
            ADD(that.first2eq, ADD(MUL(that.firstA2, that.first1eq), MUL(that.firstA3, firstV3)));
        auto secondV2 = ADD(that.second2eq,
                            ADD(MUL(that.secondA2, that.second1eq), MUL(that.secondA3, secondV3)));
        auto thirdV2 =
            ADD(that.third2eq, ADD(MUL(that.thirdA2, that.third1eq), MUL(that.thirdA3, thirdV3)));

        // ic1eq = 2 * v1 - ic1eq
        that.first1eq = SUB(MUL(that.twoSSE, firstV1), that.first1eq);
        that.second1eq = SUB(MUL(that.twoSSE, secondV1), that.second1eq);
        that.third1eq = SUB(MUL(that.twoSSE, thirdV1), that.third1eq);

        // ic2eq = 2 * v2 - ic2eq
        that.first2eq = SUB(MUL(that.twoSSE, firstV2), that.first2eq);
        that.second2eq = SUB(MUL(that.twoSSE, secondV2), that.second2eq);
        that.third2eq = SUB(MUL(that.twoSSE, thirdV2), that.third2eq);

        // results are: (m0 * input) + ((m1 * v1) + (m2 * v2))
        that.firstQueue = ADD(MUL(that.firstM0, that.firstQueue),
                              ADD(MUL(that.firstM1, firstV1), MUL(that.firstM2, firstV2)));
        that.secondQueue = ADD(MUL(that.secondM0, that.secondQueue),
                               ADD(MUL(that.secondM1, secondV1), MUL(that.secondM2, secondV2)));
        that.thirdQueue = ADD(MUL(that.thirdM0, that.thirdQueue),
                              ADD(MUL(that.thirdM1, thirdV1), MUL(that.thirdM2, thirdV2)));

        // Return the eleventh value
        float res alignas(16)[4];
        SIMD_MM(store_ps)(res, that.thirdQueue);
        vin = res[2];
    }

    // And here comes the smoothed block processing.
    // Again, we only need to worry about the Ms here.

    SIMD_M128 firstM0_prior, secondM0_prior, thirdM0_prior;
    SIMD_M128 firstM1_prior, secondM1_prior, thirdM1_prior;
    SIMD_M128 firstM2_prior, secondM2_prior, thirdM2_prior;

    SIMD_M128 firstD0, secondD0, thirdD0;
    SIMD_M128 firstD1, secondD1, thirdD1;
    SIMD_M128 firstD2, secondD2, thirdD2;

    bool firstBlock{true};

    template <int blockSize> void setCoeffForBlock(float gain)
    {
        // Preserve the prior values
        firstM0_prior = firstM0;
        secondM0_prior = secondM0;
        thirdM0_prior = thirdM0;

        firstM1_prior = firstM1;
        secondM1_prior = secondM1;
        thirdM1_prior = thirdM1;

        firstM2_prior = firstM2;
        secondM2_prior = secondM2;
        thirdM2_prior = thirdM2;

        // calculate the new M's
        setCoeff(gain);

        // If its the first time around snap them
        if (firstBlock)
        {
            firstM0_prior = firstM0;
            secondM0_prior = secondM0;
            thirdM0_prior = thirdM0;

            firstM1_prior = firstM1;
            secondM1_prior = secondM1;
            thirdM1_prior = thirdM1;

            firstM2_prior = firstM2;
            secondM2_prior = secondM2;
            thirdM2_prior = thirdM2;

            firstBlock = false;
        }

        // then for each one calculate the change across the block
        static constexpr float obsf = 1.f / blockSize;
        auto obs = SETALL(obsf);

        // and set the changeup, and reset As to the prior value so we move in the block
        firstD0 = MUL(SUB(firstM0, firstM0_prior), obs);
        secondD0 = MUL(SUB(secondM0, secondM0_prior), obs);
        thirdD0 = MUL(SUB(thirdM0, thirdM0_prior), obs);

        firstM0 = firstM0_prior;
        secondM0 = secondM0_prior;
        thirdM0 = thirdM0_prior;

        firstD1 = MUL(SUB(firstM1, firstM1_prior), obs);
        secondD1 = MUL(SUB(secondM1, secondM1_prior), obs);
        thirdD1 = MUL(SUB(thirdM1, thirdM1_prior), obs);

        firstM1 = firstM1_prior;
        secondM1 = secondM1_prior;
        thirdM1 = thirdM1_prior;

        firstD2 = MUL(SUB(firstM2, firstM2_prior), obs);
        secondD2 = MUL(SUB(secondM2, secondM2_prior), obs);
        thirdD2 = MUL(SUB(thirdM2, thirdM2_prior), obs);

        firstM2 = firstM2_prior;
        secondM2 = secondM2_prior;
        thirdM2 = thirdM2_prior;
    }

    void processBlockStep(float &input)
    {
        step(*this, input);

        firstM0 = ADD(firstM0, firstD0);
        secondM0 = ADD(secondM0, secondD0);
        thirdM0 = ADD(thirdM0, thirdD0);

        firstM1 = ADD(firstM1, firstD1);
        secondM1 = ADD(secondM1, secondD1);
        thirdM1 = ADD(thirdM1, thirdD1);

        firstM2 = ADD(firstM2, firstD2);
        secondM2 = ADD(secondM2, secondD2);
        thirdM2 = ADD(thirdM2, thirdD2);
    }

    template <int blockSize> void processBlock(const float *const input, float *output)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            output[i] = input[i];
            processBlockStep(output[i]);
        }
    }
};

#undef ADD
#undef SUB
#undef DIV
#undef MUL
#undef SETALL
#undef ZERO

} // namespace sst::filters

#endif // SHORTCIRCUITXT_CYTOMICSVF_H
