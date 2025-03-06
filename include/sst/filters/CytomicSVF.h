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

#define ADD(a,b) SIMD_MM(add_ps)(a,b)
#define SUB(a,b) SIMD_MM(sub_ps)(a,b)
#define DIV(a,b) SIMD_MM(div_ps)(a,b)
#define MUL(a,b) SIMD_MM(mul_ps)(a,b)
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
        auto v1 =
            ADD(MUL(that.a1, that.ic1eq), MUL(that.a2, v3));

        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto v2 = ADD(that.ic2eq, ADD(MUL(that.a2, that.ic1eq),
                                                              MUL(that.a3, v3)));

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

        // and set the changeup, and reset a1 to the prior value so we move in the block
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


struct FastTiltNoiseFilter
{
    /*
     Ok, so we want to feed in white noise and get it back nicely filtered to an
     N dB/oct slope for our noise colors.
     
     11 shelves in series does that really really accurately, but is a little slower than we'd like.
     
     The trick we're gonna do is actually run the filters in parallel instead of series anyway,
     passing the previous input sample on to the next filter stage with each step() call,
     and outputting the result of the 11th one each time.
     
     That means we're 11 samples latent, but that's fine. The input is just a stream of random values
     which we will generate inside this class (from an external RNG),
     so there's no phase/time relationships that we need to maintain.
     Thus we can pre-compute 11 samples on startup to get immediate output.
     
     We pay 11 extra samples worth of processing at the onset. But that seems a low price to pay for running
     the filters in parallel thereafter.
     
     We need 11 of each coefficient to run 11 filters.
     So for each coeff we have 3 m128's to fill up with the appropriate data.
     */
    
    SIMD_M128 firstQueue{ZERO()}, secondQueue{ZERO()}, thirdQueue{ZERO()};
    
    SIMD_M128 first1eq{ZERO()}, second1eq{ZERO()}, third1eq{ZERO()};
    SIMD_M128 first2eq{ZERO()}, second2eq{ZERO()}, third2eq{ZERO()};
    
    SIMD_M128 firstG, secondG, thirdG;
    SIMD_M128 kSSE;
    SIMD_M128 firstGK, secondGK, thirdGK;
    
    SIMD_M128 firstA1, secondA1, thirdA1;
    SIMD_M128 firstA2, secondA2, thirdA2;
    SIMD_M128 firstA3, secondA3, thirdA3;
    
    SIMD_M128 firstM0, secondM0, thirdM0;
    SIMD_M128 firstM1, secondM1, thirdM1;
    SIMD_M128 firstM2, secondM2, thirdM2;
    
    SIMD_M128 oneSSE{SETALL(1.0)};
    SIMD_M128 negoneSSE{SETALL(-1.0)};
    SIMD_M128 twoSSE{SETALL(2.0)};
    SIMD_M128 negtwoSSE{SETALL(-2.0)};
    
    int run{0};
    
    FastTiltNoiseFilter(float srInv)
    {
        // Since frequencies and resonances don't change in this case, a bunch of coefficients
        // can be fixed once sample rate is known and not touched after that.
       
        float freqs[11] = {20, 40, 80, 160, 320, 640, 1280, 2560, 5120, 10240, 20480};
        float G[11] = {};
        
        // g = tan(freq * srInv * pi)
        for (int i = 0; i < 11; ++i)
        {
            G[i] = sst::basic_blocks::dsp::fasttan(M_PI * freqs[i] * srInv);
            
        }

        firstG = SIMD_MM(set_ps)(G[3], G[2], G[1], G[0]);
        secondG = SIMD_MM(set_ps)(G[7], G[6], G[5], G[4]);
        thirdG = SIMD_MM(set_ps)(0.f, G[10], G[9], G[8]);
        
        // k = 2 - resonance * 2
        // which turns out to 1.86 for our intended res of .07
        kSSE = SETALL(1.86);
        
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
    }
    
    void init(sst::basic_blocks::dsp::RNG &extRng, float posGain, float negGain)
    {
        setCoeff(posGain, negGain);
        
        for (int i = 0; i < 11; ++i)
        {
            float val = extRng.unifPM1();
            step(*this, val);
        }
    }
    
    void setCoeff(float posGain, float negGain)
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
         So start by setting the first and third m128, where every element is either a low or high shelf.
         Then the middle m128 gets a blend to set its first element to the low shelf/neg gain.
         */
        
        const SIMD_M128 mask = SIMD_MM(castsi128_ps)(SIMD_MM(set_epi32)(~0,0,0,0));
        
        auto firstA = SETALL(negGain);
        auto thirdA = SETALL(posGain);
        auto secondA = SIMD_MM(blendv_ps)(thirdA, firstA, mask);
        
        firstM0 = oneSSE;
        thirdM0 = MUL(thirdA, thirdA);
        secondM0 = SIMD_MM(blendv_ps)(thirdM0, firstM0, mask);
        
        firstM1 = MUL(kSSE, SUB(firstA, oneSSE));
        thirdM1 = MUL(MUL(kSSE, SUB(oneSSE, thirdA)), thirdA);
        secondM1 = SIMD_MM(blendv_ps)(thirdM1, firstM1, mask);
        
        firstM2 = SUB(MUL(firstA, firstA), oneSSE);
        thirdM2 = SUB(oneSSE, MUL(firstA, firstA));
        secondM2 = SIMD_MM(blendv_ps)(thirdM2, firstM2, mask);
    }
    
    static void step(FastTiltNoiseFilter &that, float &vin)
    {
        // The shuffle algo works. Filter algo does not.
        
        
        
        float tQ1 alignas(16)[4];
        float tQ2 alignas(16)[4];
        float tQ3 alignas(16)[4];
        SIMD_MM(store_ps)(tQ1, that.firstQueue);
        SIMD_MM(store_ps)(tQ2, that.secondQueue);
        SIMD_MM(store_ps)(tQ3, that.thirdQueue);
        
        // Shuffle the queues forwards, adding in the current input
        that.firstQueue = SIMD_MM(set_ps)(tQ1[2], tQ1[1], tQ1[0], vin);
        that.secondQueue = SIMD_MM(set_ps)(tQ2[2], tQ2[1], tQ2[0], tQ1[3]);
        that.thirdQueue = SIMD_MM(set_ps)(tQ3[2], tQ3[1], tQ3[0], tQ2[3]);
        
        if (that.run < 12)
        {
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, that.firstQueue);
            SIMD_MM(store_ps)(dQ2, that.secondQueue);
            SIMD_MM(store_ps)(dQ3, that.thirdQueue);
            
            std::cout << "Queue beforewards =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // time to run the filter algo
        
        // v3 = input (aka v0) - ic2eq
        auto firstV3 = SUB(that.firstQueue, that.first2eq);
        auto secondV3 = SUB(that.secondQueue, that.second2eq);
        auto thirdV3 = SUB(that.thirdQueue, that.third2eq);
        
        if (that.run < 12)
        {
            std::cout << "input = " << vin << std::endl;
            
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, firstV3);
            SIMD_MM(store_ps)(dQ2, secondV3);
            SIMD_MM(store_ps)(dQ3, thirdV3);
            
            std::cout << "V3 =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // v1 = a1 * ic1eq + a2 * v3
        auto firstV1 = ADD(MUL(that.firstA1, that.first1eq), MUL(that.firstA2, firstV3));
        auto secondV1 = ADD(MUL(that.secondA1, that.second1eq), MUL(that.secondA2, secondV3));
        auto thirdV1 = ADD(MUL(that.thirdA1, that.third1eq), MUL(that.thirdA2, thirdV3));
        
        if (that.run < 12)
        {
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, firstV1);
            SIMD_MM(store_ps)(dQ2, secondV1);
            SIMD_MM(store_ps)(dQ3, thirdV1);
            
            std::cout << "V1 =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto firstV2 = ADD(that.first2eq, ADD(MUL(that.firstA2, that.first1eq), MUL(that.firstA3, firstV3)));
        auto secondV2 = ADD(that.second2eq, ADD(MUL(that.secondA2, that.second1eq), MUL(that.secondA3, secondV3)));
        auto thirdV2 = ADD(that.third2eq, ADD(MUL(that.thirdA2, that.third1eq), MUL(that.thirdA3, thirdV3)));
        
        if (that.run < 12)
        {
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, firstV2);
            SIMD_MM(store_ps)(dQ2, secondV2);
            SIMD_MM(store_ps)(dQ3, thirdV2);
            
            std::cout << "V2 =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }

        // ic1eq = 2 * v1 - ic1eq
        that.first1eq = SUB(MUL(that.twoSSE, firstV1), that.first1eq);
        that.second1eq = SUB(MUL(that.twoSSE, secondV1), that.second1eq);
        that.third1eq = SUB(MUL(that.twoSSE, thirdV1), that.third1eq);
        
        if (that.run < 12)
        {
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, that.first1eq);
            SIMD_MM(store_ps)(dQ2, that.second1eq);
            SIMD_MM(store_ps)(dQ3, that.third1eq);
            
            std::cout << "1EQ =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // ic2eq = 2 * v2 - ic2eq
        that.first2eq = SUB(MUL(that.twoSSE, firstV2), that.first2eq);
        that.second2eq = SUB(MUL(that.twoSSE, secondV2), that.second2eq);
        that.third2eq = SUB(MUL(that.twoSSE, thirdV2), that.third2eq);
        
        if (that.run < 12)
        {
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, that.first2eq);
            SIMD_MM(store_ps)(dQ2, that.second2eq);
            SIMD_MM(store_ps)(dQ3, that.third2eq);
            
            std::cout << "2EQ =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // results are:
        // (m0 * input) + ((m1 * v1) + (m2 * v2))
        that.firstQueue = ADD(MUL(that.firstM0, that.firstQueue), ADD(MUL(that.firstM1, firstV1), MUL(that.firstM2, firstV2)));
        that.secondQueue = ADD(MUL(that.secondM0, that.secondQueue), ADD(MUL(that.secondM1, secondV1), MUL(that.secondM2, secondV2)));
        that.thirdQueue = ADD(MUL(that.thirdM0, that.thirdQueue), ADD(MUL(that.thirdM1, thirdV1), MUL(that.thirdM2, thirdV2)));
        
        if (that.run < 12)
        {
            // DEBUG STATEMENT
            
            float dQ1 alignas(16)[4];
            float dQ2 alignas(16)[4];
            float dQ3 alignas(16)[4];
            SIMD_MM(store_ps)(dQ1, that.firstQueue);
            SIMD_MM(store_ps)(dQ2, that.secondQueue);
            SIMD_MM(store_ps)(dQ3, that.thirdQueue);
            
            std::cout << "Queue afterwards =";
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ1[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ2[i] << ", ";
            }
            for (int i = 0; i < 4; ++i)
            {
                std::cout << " " << dQ3[i] << ", ";
            }
            std::cout << std::endl;
        }
        
        // Return the eleventh value
        
        float res alignas(16)[4];
        SIMD_MM(store_ps)(res, that.thirdQueue);
        vin = res[2];
        
        if (that.run < 12)
        {
            std::cout << "output = " << vin << std::endl;
            std::cout << std::endl;
            that.run += 1;
        }
         
    }
    /*
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
        auto obs = SETALL(obsf);

        // and set the changeup, and reset a1 to the prior value so we move in the block
        firstDA1 = MUL(SUB(firstA1, firstA1_prior), obs);
        secondDA1 = MUL(SUB(secondA1, secondA1_prior), obs);
        thirdDA1 = MUL(SUB(thirdA1, thirdA1_prior), obs);
        
        firstA1 = firstA1_prior;
        secondA1 = secondA1_prior;
        thirdA1 = thirdA1_prior;
        
        firstDA2 = MUL(SUB(firstA2, firstA2_prior), obs);
        secondDA2 = MUL(SUB(secondA2, secondA2_prior), obs);
        thirdDA2 = MUL(SUB(thirdA2, thirdA2_prior), obs);
        
        firstA2 = firstA2_prior;
        secondA2 = secondA2_prior;
        thirdA2 = thirdA2_prior;

        firstDA3 = MUL(SUB(firstA3, firstA3_prior), obs);
        secondDA3 = MUL(SUB(secondA3, secondA3_prior), obs);
        thirdDA3 = MUL(SUB(thirdA3, thirdA3_prior), obs);
        
        firstA3 = firstA3_prior;
        secondA3 = secondA3_prior;
        thirdA3 = thirdA3_prior;
    }
    
    void processBlockStep(float &input)
    {
        step(*this, input);
        
        firstA1 = ADD(firstA1, firstDA1);
        secondA1 = ADD(secondA1, secondDA1);
        thirdA1 = ADD(thirdA1, thirdDA1);
        
        firstA2 = ADD(firstA2, firstDA2);
        secondA2 = ADD(secondA2, secondDA2);
        thirdA2 = ADD(thirdA2, thirdDA2);
        
        firstA3 = ADD(firstA3, firstDA3);
        secondA3 = ADD(secondA3, secondDA3);
        thirdA3 = ADD(thirdA3, thirdDA3);
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
     */
};

#undef ADD
#undef SUB
#undef DIV
#undef MUL
#undef SETALL
#undef ZERO

} // namespace sst::filters

#endif // SHORTCIRCUITXT_CYTOMICSVF_H
