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
 * for you to copy in an MIT licensed context. Basically
 * go ahead and use it however you want, in the same spirit as Andy's
 * sharing the work.
 */

#ifndef INCLUDE_SST_FILTERS_FASTTILTNOISEFILTER_H
#define INCLUDE_SST_FILTERS_FASTTILTNOISEFILTER_H

#include <algorithm>
#include <cmath>
#include <cassert>

#include "sst/basic-blocks/simd/setup.h"
#include "sst/basic-blocks/mechanics/simd-ops.h"

// FastTiltNoiseFilter::step uses SSE4.1 ops (alignr_epi8 from SSSE3 plus
// other intrinsics covered by SSE4.1). On x86 with GCC/Clang those
// intrinsics are gated on the calling function's target ISA — without
// -msse4.1 (or higher, e.g. -mavx) GCC will refuse to inline them and
// emit a "target specific option mismatch" error deep in tmmintrin.h.
// Catch that here with a clear message instead. MSVC always exposes the
// intrinsics, and on non-x86 we go through simde which is target-flag
// independent, so the check is x86-GCC/Clang only.
#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER) &&                             \
    (defined(__x86_64__) || defined(__i386__)) && !defined(__SSE4_1__)
#error                                                                                             \
    "FastTiltNoiseFilter requires SSE4.1 on x86. Pass -msse4.1 (or higher, e.g. -mavx) on the compile line for any translation unit that includes this header."
#endif

namespace sst::filters
{

#define ADD(a, b) SIMD_MM(add_ps)(a, b)
#define SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define DIV(a, b) SIMD_MM(div_ps)(a, b)
#define MUL(a, b) SIMD_MM(mul_ps)(a, b)
#define SETALL(a) SIMD_MM(set1_ps)(a)

template <typename hostClass> struct FastTiltNoiseFilter
{
    /*
     Ok, for our noise colors we want to feed in white noise,
     and get it back nicely filtered to an N dB/oct slope.

     11 of the Cytomic SVF filters in series can do that really really accurately.
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

    SIMD_M128 firstQueue{SIMD_MM(setzero_ps)()}, secondQueue{SIMD_MM(setzero_ps)()},
        thirdQueue{SIMD_MM(setzero_ps)()};

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

    SIMD_M128 first1eq{SIMD_MM(setzero_ps)()}, second1eq{SIMD_MM(setzero_ps)()},
        third1eq{SIMD_MM(setzero_ps)()};
    SIMD_M128 first2eq{SIMD_MM(setzero_ps)()}, second2eq{SIMD_MM(setzero_ps)()},
        third2eq{SIMD_MM(setzero_ps)()};

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
            auto freq = std::pow(2, i + 1) * 10.f;

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
        auto secondA =
            SIMD_MM(add_ps)(SIMD_MM(and_ps)(mask, thirdA), SIMD_MM(andnot_ps)(mask, firstA));

        // m0 = 1 for low shelf, A^2 for high
        firstM0 = oneSSE;
        thirdM0 = MUL(thirdA, thirdA);
        secondM0 =
            SIMD_MM(add_ps)(SIMD_MM(and_ps)(mask, thirdM0), SIMD_MM(andnot_ps)(mask, firstM0));

        // m1 = k * (A - 1) for low shelf, (k * (1 - A)) * A for high
        firstM1 = MUL(kSSE, SUB(firstA, oneSSE));
        thirdM1 = MUL(MUL(kSSE, SUB(oneSSE, thirdA)), thirdA);
        secondM1 =
            SIMD_MM(add_ps)(SIMD_MM(and_ps)(mask, thirdM1), SIMD_MM(andnot_ps)(mask, firstM1));

        // m2 = A^2 - 1 for low shelf, 1 - A^2 for high
        firstM2 = SUB(MUL(firstA, firstA), oneSSE);
        thirdM2 = SUB(oneSSE, MUL(thirdA, thirdA));
        secondM2 =
            SIMD_MM(add_ps)(SIMD_MM(and_ps)(mask, thirdM2), SIMD_MM(andnot_ps)(mask, firstM2));
    }

    static void step(FastTiltNoiseFilter &that, float &vin)
    {
        // Shift the 12-lane queue [firstQueue : secondQueue : thirdQueue] up
        // by one float, drop `vin` into the new lowest lane, and let the top
        // lane fall off. Conceptually:
        //   new firstQueue  = { vin,           oldFirst[0..2]  }
        //   new secondQueue = { oldFirst[3],   oldSecond[0..2] }
        //   new thirdQueue  = { oldSecond[3],  oldThird[0..2]  }
        //
        // The previous version did three store_ps + three set_ps, which round-
        // trips every lane through the stack. Here we stay register-resident.

        auto oldFirst = that.firstQueue;
        auto oldSecond = that.secondQueue;

        // firstQueue is special because its new lane 0 is a scalar (vin), not
        // a lane from another vector.
        //
        // shuffle_ps(a, a, _MM_SHUFFLE(2,1,0,0)) lifts oldFirst's lanes up by
        // one, duplicating lane 0 into both lane 0 and lane 1:
        //   { oldFirst[0], oldFirst[0], oldFirst[1], oldFirst[2] }
        // Lane 0 there is just a placeholder. move_ss then overwrites lane 0
        // with vin (move_ss copies only the bottom float, leaving lanes 1..3).
        auto shifted1 = SIMD_MM(shuffle_ps)(oldFirst, oldFirst, SIMD_MM_SHUFFLE(2, 1, 0, 0));
        that.firstQueue = SIMD_MM(move_ss)(shifted1, SIMD_MM(set_ss)(vin));

        // secondQueue and thirdQueue both want "shift up by one and splice in
        // the previous queue's top lane in lane 0". That is exactly what
        // alignr_epi8 (SSSE3) does:
        //
        //   alignr_epi8(hi, lo, n) treats {hi : lo} as 32 bytes, shifts right
        //   by n bytes, and returns the low 16 bytes. With n = 12 (3 floats):
        //     result = { lo[3], hi[0], hi[1], hi[2] }
        //
        // alignr_epi8 lives in the integer domain (epi8 == "extended packed
        // int 8"), so we cast __m128 <-> __m128i around it. The casts are
        // bitwise reinterprets and compile to nothing.
        that.secondQueue = SIMD_MM(castsi128_ps)(SIMD_MM(alignr_epi8)(
            SIMD_MM(castps_si128)(oldSecond), SIMD_MM(castps_si128)(oldFirst), 12));
        that.thirdQueue = SIMD_MM(castsi128_ps)(SIMD_MM(alignr_epi8)(
            SIMD_MM(castps_si128)(that.thirdQueue), SIMD_MM(castps_si128)(oldSecond), 12));

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

        // Return the eleventh value, which lives in lane 2 of thirdQueue.
        // simd_float_extract<N> is a register-resident extract (shuffle +
        // cvtss_f32 under the hood) — no stack round-trip.
        vin = sst::basic_blocks::mechanics::simd_float_extract<2>(that.thirdQueue);
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

} // namespace sst::filters

#endif // SHORTCIRCUITXT_FASTTILT_H
