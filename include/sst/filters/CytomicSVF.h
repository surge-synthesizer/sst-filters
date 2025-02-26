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
        auto obs = SIMD_MM(set1_ps)(obsf);

        // and set the changeup, and reset a1 to the prior value so we move in the block
        da1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a1, a1_prior), obs);
        a1 = a1_prior;

        da2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a2, a2_prior), obs);
        a2 = a2_prior;

        da3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a3, a3_prior), obs);
        a3 = a3_prior;
        
        dm0 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m0, m0_prior), obs);
        m0 = m0_prior;

        dm1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m1, m1_prior), obs);
        m1 = m1_prior;

        dm2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m2, m2_prior), obs);
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
        auto obs = SIMD_MM(set1_ps)(obsf);

        
        da1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a1, a1_prior), obs);
        a1 = a1_prior;

        da2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a2, a2_prior), obs);
        a2 = a2_prior;

        da3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(a3, a3_prior), obs);
        a3 = a3_prior;
        
        dm0 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m0, m0_prior), obs);
        m0 = m0_prior;

        dm1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m1, m1_prior), obs);
        m1 = m1_prior;

        dm2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(m2, m2_prior), obs);
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
        a1 = SIMD_MM(add_ps)(a1, da1);
        a2 = SIMD_MM(add_ps)(a2, da2);
        a3 = SIMD_MM(add_ps)(a3, da3);
        m1 = SIMD_MM(add_ps)(m1, dm1);
        m2 = SIMD_MM(add_ps)(m2, dm2);
        m0 = SIMD_MM(add_ps)(m0, dm0);
    }

    void processBlockStep(float &L)
    {
        float tmp{0.f};
        step(*this, L, tmp);
        a1 = SIMD_MM(add_ps)(a1, da1);
        a2 = SIMD_MM(add_ps)(a2, da2);
        a3 = SIMD_MM(add_ps)(a3, da3);
        m1 = SIMD_MM(add_ps)(m1, dm1);
        m2 = SIMD_MM(add_ps)(m2, dm2);
        m0 = SIMD_MM(add_ps)(m0, dm0);
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


struct FastTiltNoiseFilter
{
    SIMD_M128 firstQueue{SIMD_MM(setzero_ps)()}, secondQueue{SIMD_MM(setzero_ps)()}, thirdQueue{SIMD_MM(setzero_ps)()};
    
    SIMD_M128 first1eq{SIMD_MM(setzero_ps)()}, second1eq{SIMD_MM(setzero_ps)()}, third1eq{SIMD_MM(setzero_ps)()};
    SIMD_M128 first2eq{SIMD_MM(setzero_ps)()}, second2eq{SIMD_MM(setzero_ps)()}, third2eq{SIMD_MM(setzero_ps)()};
    
    SIMD_M128 firstG, secondG, thirdG;
    SIMD_M128 kSSE;
    SIMD_M128 firstGK, secondGK, thirdGK;
    
    SIMD_M128 firstA1, secondA1, thirdA1;
    SIMD_M128 firstA2, secondA2, thirdA2;
    SIMD_M128 firstA3, secondA3, thirdA3;
    
    SIMD_M128 firstM0, secondM0, thirdM0;
    SIMD_M128 firstM1, secondM1, thirdM1;
    SIMD_M128 firstM2, secondM2, thirdM2;
    
    SIMD_M128 oneSSE{SIMD_MM(set1_ps)(1.0)};
    SIMD_M128 negoneSSE{SIMD_MM(set1_ps)(-1.0)};
    SIMD_M128 twoSSE{SIMD_MM(set1_ps)(2.0)};
    SIMD_M128 negtwoSSE{SIMD_MM(set1_ps)(-2.0)};
    
    int run{0};
    
    float srInv{};
    
    FastTiltNoiseFilter(float sri) : srInv(sri)
    {
        namespace sdsp = sst::basic_blocks::dsp;
        auto sriXpi = M_PI * srInv;
        
        /*
         Since frequencies and resonances don't change in this case, a bunch of coefficients
         can be fixed once sample rate is known and not touched after that.
         
         k is initialized above, it's:
         k = 2 - resonance * 2
         which turns out to 1.86 for our intended res of .07
         
         We need eleven of each of those to run 11 filters.
         So make 3 m128's which we fill up with the appropriate data.
         The last element of the third G won't get used really.
         */
        
        // g = tan(freq*srInv*pi)
        firstG = SIMD_MM(set_ps)(sdsp::fasttan(20 * sriXpi),
                                            sdsp::fasttan(40 * sriXpi),
                                            sdsp::fasttan(80 * sriXpi),
                                            sdsp::fasttan(160 * sriXpi)
                                            );
        secondG = SIMD_MM(set_ps)(sdsp::fasttan(320 * sriXpi),
                                             sdsp::fasttan(640 * sriXpi),
                                             sdsp::fasttan(1280 * sriXpi),
                                             sdsp::fasttan(2560 * sriXpi)
                                             );
        thirdG = SIMD_MM(set_ps)(sdsp::fasttan(5120 * sriXpi),
                                            sdsp::fasttan(10240 * sriXpi),
                                            sdsp::fasttan(20480 * sriXpi),
                                            0.4999f);
        
        kSSE = SIMD_MM(set1_ps)(1.86);
        
        // gk = g + k
        firstGK = SIMD_MM(add_ps)(firstG, kSSE);
        secondGK = SIMD_MM(add_ps)(secondG, kSSE);
        thirdGK = SIMD_MM(add_ps)(thirdG, kSSE);
        
        // a1 = 1 / (1 + g * gk)
        firstA1 = SIMD_MM(div_ps)(oneSSE, SIMD_MM(add_ps)(oneSSE, SIMD_MM(mul_ps)(firstG, firstGK)));
        secondA1 = SIMD_MM(div_ps)(oneSSE, SIMD_MM(add_ps)(oneSSE, SIMD_MM(mul_ps)(secondG, secondGK)));
        thirdA1 = SIMD_MM(div_ps)(oneSSE, SIMD_MM(add_ps)(oneSSE, SIMD_MM(mul_ps)(thirdG, thirdGK)));
        
        // a2 = g * a1
        firstA2 = SIMD_MM(mul_ps)(firstG, firstA1);
        secondA2 = SIMD_MM(mul_ps)(secondG, secondA1);
        thirdA2 = SIMD_MM(mul_ps)(thirdG, thirdA1);
        
        // a3 = g * a2
        firstA3 = SIMD_MM(mul_ps)(firstG, firstA2);
        secondA3 = SIMD_MM(mul_ps)(secondG, secondA2);
        thirdA3 = SIMD_MM(mul_ps)(thirdG, thirdA2);
    }
    
    void init(sst::basic_blocks::dsp::RNG &extRng, float initialGain = 1)
    {
        // see further comments in setCoeff()...
        setCoeff(initialGain);
        // and step()...
        for (int i = 0; i < 11; ++i)
        {
            float val = extRng.unifPM1();
            step(*this, val);
        }
    }
    
    void setCoeff(float gain)
    {
        /*
         The only coefficients that change in this class are the gain-related ones.
         A = gain.
         Then for a low shelf we need:
         m0 = 1
         m1 = k * (A - 1)
         m2 = A^2 - 1
         
         And for the high shelf:
         m0 = A^2
         m1 = (k * (1 - A)) * A
         m2 = 1 - A^2
        
         Five of the filters are low shelves and have their gain inverted, the rest are high shelves.
         So start by setting the first and third m128, where every element is either low or high shelf.
         Then the middle m128 gets a blend to set its first element to the low shelf/neg gain.
         */
        
        auto firstA = SIMD_MM(set1_ps)(-gain);
        auto thirdA = SIMD_MM(set1_ps)(gain);
        auto secondA = SIMD_MM(blend_ps)(thirdA, firstA, 0);
        
        firstM0 = oneSSE;
        thirdM0 = SIMD_MM(sub_ps)(oneSSE, SIMD_MM(mul_ps)(thirdA, thirdA));
        secondM0 = SIMD_MM(blend_ps)(thirdM0, firstM0, 0);
        
        firstM1 = SIMD_MM(mul_ps)(kSSE, SIMD_MM(sub_ps)(firstA, oneSSE));
        thirdM1 = SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(kSSE, SIMD_MM(sub_ps)(oneSSE, thirdA)), thirdA);
        secondM1 = SIMD_MM(blend_ps)(thirdM1, firstM0, 0);
        
        firstM2 = SIMD_MM(sub_ps)(oneSSE, SIMD_MM(mul_ps)(firstA, firstA));
        thirdM2 = SIMD_MM(sub_ps)(oneSSE, SIMD_MM(mul_ps)(firstA, firstA));
        secondM2 = SIMD_MM(blend_ps)(thirdM2, firstM2, 0);
    }
    
    static void step(FastTiltNoiseFilter &that, float &vin)
    {
        /* Each sample we feed in a new random value, and we want one back which
         should have gone through 11 shelves in series.
         
         What we're gonna do here is actually run the filters in parallel anyway, passing an input sample
         to the next filter stage with each step() call.
         
         That means we're 11 samples latent, but that's fine. The input is just noise, so we
         can pre-compute 11 samples on startup and get immediate output anyway!
         
         11 extra samples worth of processing in the constructor is a low price to pay for running
         all 11 filters in parallel thereafter.
         */
        
        if (that.run < 12)
        {
            std::cout << "input = " << vin << std::endl;
        }
        
        float tQ1 alignas(16)[4];
        float tQ2 alignas(16)[4];
        float tQ3 alignas(16)[4];
        SIMD_MM(store_ps)(tQ1, that.firstQueue);
        SIMD_MM(store_ps)(tQ2, that.secondQueue);
        SIMD_MM(store_ps)(tQ3, that.thirdQueue);
        
        if (that.run < 12)
        {
            std::cout << "registers are =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << tQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << tQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << tQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // Shuffle the queues forwards, adding in the current input
        that.firstQueue = SIMD_MM(setr_ps)(vin, tQ1[0], tQ1[1], tQ1[2]);
        that.secondQueue = SIMD_MM(setr_ps)(tQ1[3], tQ2[0], tQ2[1], tQ2[2]);
        that.thirdQueue = SIMD_MM(setr_ps)(tQ2[3], tQ3[0], tQ3[1], tQ3[2]);
        
        // time to run the filter algo
        
        // v3 = input (aka v0) - ic2eq
        auto firstV3 = SIMD_MM(sub_ps)(that.firstQueue, that.first1eq);
        auto secondV3 = SIMD_MM(sub_ps)(that.secondQueue, that.second1eq);
        auto thirdV3 = SIMD_MM(sub_ps)(that.thirdQueue, that.third1eq);
        
        // v1 = a1 * ic1eq + a2 * v3
        auto firstV1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.firstA1, that.first1eq), SIMD_MM(mul_ps)(that.firstA2, firstV3));
        auto secondV1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.secondA1, that.second1eq), SIMD_MM(mul_ps)(that.secondA2, secondV3));
        auto thirdV1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.thirdA1, that.third1eq), SIMD_MM(mul_ps)(that.thirdA2, thirdV3));
        
        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto firstV2 = SIMD_MM(add_ps)(that.first2eq, SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.firstA2, that.first1eq), SIMD_MM(mul_ps)(that.firstA3, firstV3)));
        auto secondV2 = SIMD_MM(add_ps)(that.second2eq, SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.secondA2, that.second1eq), SIMD_MM(mul_ps)(that.secondA3, secondV3)));
        auto thirdV2 = SIMD_MM(add_ps)(that.third2eq, SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.thirdA2, that.third1eq), SIMD_MM(mul_ps)(that.thirdA3, thirdV3)));
        
        // ic1eq = 2 * v1 - ic1eq
        that.first1eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, firstV1), that.first1eq);
        that.second1eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, secondV1), that.second1eq);
        that.third1eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, thirdV1), that.third1eq);
        
        // ic2eq = 2 * v2 - ic2eq
        that.first2eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, firstV2), that.first2eq);
        that.second2eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, secondV2), that.second2eq);
        that.third2eq = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(that.twoSSE, thirdV2), that.third2eq);
        
        // results are:
        // (m0 * input) + ((m1 * v1) + (m2 * v2))
        that.firstQueue = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.firstM0, that.firstQueue), SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.firstM1, firstV1), SIMD_MM(mul_ps)(that.firstM2, firstV2)));
        that.secondQueue = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.secondM0, that.secondQueue), SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.secondM1, secondV1), SIMD_MM(mul_ps)(that.secondM2, secondV2)));
        that.thirdQueue = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.thirdM0, that.thirdQueue), SIMD_MM(add_ps)(SIMD_MM(mul_ps)(that.thirdM1, thirdV1), SIMD_MM(mul_ps)(that.thirdM2, thirdV2)));
        
        // Return the eleventh value
        
        float res alignas(16)[4];
        SIMD_MM(store_ps)(res, that.thirdQueue);
        vin = res[2];
        if (that.run < 12)
        {
            std::cout << "output = " << vin << std::endl;
            that.run += 1;
        }
    }
    
    // Block processing with smoothing:
    
    SIMD_M128 firstA1_prior, secondA1_prior, thirdA1_prior;
    SIMD_M128 firstA2_prior, secondA2_prior, thirdA2_prior;
    SIMD_M128 firstA3_prior, secondA3_prior, thirdA3_prior;
    
    SIMD_M128 firstDA1, secondDA1, thirdDA1;
    SIMD_M128 firstDA2, secondDA2, thirdDA2;
    SIMD_M128 firstDA3, secondDA3, thirdDA3;
    
    bool firstBlock{true};
    
    template <int blockSize>
    void setCoeffForBlock(float gain)
    {
        // Preserve the prior values
        firstA1_prior = firstA1;
        secondA1_prior = secondA1;
        thirdA1_prior = thirdA1;
        
        firstA2_prior = firstA2;
        secondA2_prior = secondA2;
        thirdA2_prior = thirdA2;
        
        firstA3_prior = firstA3;
        secondA3_prior = secondA3;
        thirdA3_prior = thirdA3;

        // calculate the new a1s
        setCoeff(gain);

        // If its the first time around snap them
        if (firstBlock)
        {
            firstA1_prior = firstA1;
            secondA1_prior = secondA1;
            thirdA1_prior = thirdA1;
            
            firstA2_prior = firstA2;
            secondA2_prior = secondA2;
            thirdA2_prior = thirdA2;
            
            firstA3_prior = firstA3;
            secondA3_prior = secondA3;
            thirdA3_prior = thirdA3;
            
            firstBlock = false;
        }

        // then for each one calculate the change across the block
        static constexpr float obsf = 1.f / blockSize;
        auto obs = SIMD_MM(set1_ps)(obsf);

        // and set the changeup, and reset a1 to the prior value so we move in the block
        firstDA1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(firstA1, firstA1_prior), obs);
        secondDA1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(secondA1, secondA1_prior), obs);
        thirdDA1 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(thirdA1, thirdA1_prior), obs);
        
        firstA1 = firstA1_prior;
        secondA1 = secondA1_prior;
        thirdA1 = thirdA1_prior;
        
        firstDA2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(firstA2, firstA2_prior), obs);
        secondDA2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(secondA2, secondA2_prior), obs);
        thirdDA2 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(thirdA2, thirdA2_prior), obs);
        
        firstA2 = firstA2_prior;
        secondA2 = secondA2_prior;
        thirdA2 = thirdA2_prior;

        firstDA3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(firstA3, firstA3_prior), obs);
        secondDA3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(secondA3, secondA3_prior), obs);
        thirdDA3 = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(thirdA3, thirdA3_prior), obs);
        
        firstA3 = firstA3_prior;
        secondA3 = secondA3_prior;
        thirdA3 = thirdA3_prior;
    }
    
    void processBlockStep(float &input)
    {
        step(*this, input);
        
        firstA1 = SIMD_MM(add_ps)(firstA1, firstDA1);
        secondA1 = SIMD_MM(add_ps)(secondA1, secondDA1);
        thirdA1 = SIMD_MM(add_ps)(thirdA1, thirdDA1);
        
        firstA2 = SIMD_MM(add_ps)(firstA2, firstDA2);
        secondA2 = SIMD_MM(add_ps)(secondA2, secondDA2);
        thirdA2 = SIMD_MM(add_ps)(thirdA2, thirdDA2);
        
        firstA3 = SIMD_MM(add_ps)(firstA3, firstDA3);
        secondA3 = SIMD_MM(add_ps)(secondA3, secondDA3);
        thirdA3 = SIMD_MM(add_ps)(thirdA3, thirdDA3);
    }
    
    template <int blockSize>
    void processBlock(const float *const input, float *output)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            output[i] = input[i];
            processBlockStep(output[i]);
        }
    }
};

} // namespace sst::filters

#endif // SHORTCIRCUITXT_CYTOMICSVF_H
