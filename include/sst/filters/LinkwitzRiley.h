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

#ifndef INCLUDE_SST_FILTERS_LINKWITZRILEY_H
#define INCLUDE_SST_FILTERS_LINKWITZRILEY_H

#include <array>

#include "CytomicSVF.h"

/*
 * A 4th order Linkwitz-Riley (LR4) stereo crossover built on the CytomicSVF.
 *
 * An LR4 crossover splits a signal into a low band and a high band such that
 * the two bands sum back to the input with a flat magnitude response (an
 * allpass). The bands stay in phase at every frequency and are each down 6 dB
 * at the crossover, so nothing is boosted or notched when you recombine them.
 *
 * An LR4 section is a cascade of two identical 2nd order Butterworth sections
 * (Q = 1/sqrt(2)). The CytomicSVF maps its resonance to k = 2 - 2*res with
 * Q = 1/k, so a Butterworth section needs k = sqrt(2), i.e.
 *
 *     2 - 2*res = sqrt(2)   =>   res = (2 - sqrt(2)) / 2 = 1 - 1/sqrt(2)
 *
 * The neat part is the SIMD packing. We load a stereo sample into one register
 * as [L, R, L, R] and assign the two lower lanes the lowpass mode and the two
 * upper lanes the highpass mode, so a single SVF step produces both bands of
 * both channels at once. Two such steps cascaded give LR4. Both cascade stages
 * use *identical* coefficients (same cutoff, same Butterworth res, same modes)
 * so we compute them once and share them via fetchCoeffs. Within a stage the
 * lowpass and highpass lanes share g/k/a1/a2/a3 and differ only in the m0/m1/m2
 * output mix, so there is no duplicated coefficient math at all.
 */

namespace sst::filters
{

struct LinkwitzRileyLR4Crossover
{
    // Two cascaded Butterworth sections make up the LR4. They are coefficient
    // clones of each other; only their integrator state differs.
    CytomicSVF stage1, stage2;

    // res that turns a CytomicSVF section into a Butterworth (Q = 1/sqrt(2))
    // section: 1 - 1/sqrt(2). Spelled out so we do not lean on M_SQRT1_2.
    static constexpr float butterworthRes{0.2928932188134524f};

    // [L, R, L, R] -> lowpass on lanes 0,1 and highpass on lanes 2,3
    static constexpr std::array<CytomicSVF::Mode, 4> crossoverModes{
        CytomicSVF::Mode::Lowpass, CytomicSVF::Mode::Lowpass, CytomicSVF::Mode::Highpass,
        CytomicSVF::Mode::Highpass};

    void init()
    {
        stage1.init();
        stage2.init();
    }

    /*
     * Set the crossover frequency directly (no smoothing). Both cascade stages
     * receive the same coefficients; stage2 fetches them rather than recomputing.
     */
    void setCoeff(float crossoverFreq, float srInv)
    {
        stage1.setCoeff(crossoverModes, crossoverFreq, butterworthRes, srInv);
        stage2.fetchCoeffs(stage1);
    }

    /*
     * One stereo sample in, both bands of both channels out. The lowpass and
     * highpass bands are in phase, so low + high reconstructs the input.
     */
    void step(float L, float R, float &lowL, float &lowR, float &highL, float &highR)
    {
        auto vin = SIMD_MM(set_ps)(R, L, R, L); // [L, R, L, R]
        auto s1 = CytomicSVF::stepSSE(stage1, vin);
        auto s2 = CytomicSVF::stepSSE(stage2, s1);

        float r4 alignas(16)[4];
        SIMD_MM(store_ps)(r4, s2);
        lowL = r4[0];
        lowR = r4[1];
        highL = r4[2];
        highR = r4[3];
    }

    /*
     * Block processing with per-sample coefficient smoothing across the block.
     * Only g/a1/a2/a3 move as the cutoff glides; the modes (and hence k and the
     * m mix) are fixed, so we smooth a1/a2/a3 and keep both stages in lockstep.
     */
#define LR_ADD(a, b) SIMD_MM(add_ps)(a, b)
#define LR_SUB(a, b) SIMD_MM(sub_ps)(a, b)
#define LR_MUL(a, b) SIMD_MM(mul_ps)(a, b)

    SIMD_M128 da1{SIMD_MM(setzero_ps)()}, da2{SIMD_MM(setzero_ps)()}, da3{SIMD_MM(setzero_ps)()};
    bool firstBlock{true};

    template <int blockSize> void setCoeffForBlock(float crossoverFreq, float srInv)
    {
        auto a1p = stage1.a1;
        auto a2p = stage1.a2;
        auto a3p = stage1.a3;

        setCoeff(crossoverFreq, srInv);

        if (firstBlock)
        {
            a1p = stage1.a1;
            a2p = stage1.a2;
            a3p = stage1.a3;
            firstBlock = false;
        }

        static constexpr float obsf = 1.f / blockSize;
        auto obs = SIMD_MM(set1_ps)(obsf);

        da1 = LR_MUL(LR_SUB(stage1.a1, a1p), obs);
        da2 = LR_MUL(LR_SUB(stage1.a2, a2p), obs);
        da3 = LR_MUL(LR_SUB(stage1.a3, a3p), obs);

        setStageA(a1p, a2p, a3p);
    }

    template <int blockSize> void retainCoeffForBlock()
    {
        da1 = SIMD_MM(setzero_ps)();
        da2 = SIMD_MM(setzero_ps)();
        da3 = SIMD_MM(setzero_ps)();
    }

    void processBlockStep(float L, float R, float &lowL, float &lowR, float &highL, float &highR)
    {
        step(L, R, lowL, lowR, highL, highR);
        setStageA(LR_ADD(stage1.a1, da1), LR_ADD(stage1.a2, da2), LR_ADD(stage1.a3, da3));
    }

    template <int blockSize>
    void processBlock(const float *const inL, const float *const inR, float *lowL, float *lowR,
                      float *highL, float *highR)
    {
        for (int i = 0; i < blockSize; ++i)
            processBlockStep(inL[i], inR[i], lowL[i], lowR[i], highL[i], highR[i]);
    }

  private:
    // keep the two stages' a-coefficients identical
    void setStageA(SIMD_M128 a1, SIMD_M128 a2, SIMD_M128 a3)
    {
        stage1.a1 = a1;
        stage1.a2 = a2;
        stage1.a3 = a3;
        stage2.a1 = a1;
        stage2.a2 = a2;
        stage2.a3 = a3;
    }

#undef LR_ADD
#undef LR_SUB
#undef LR_MUL
};

} // namespace sst::filters

#endif // INCLUDE_SST_FILTERS_LINKWITZRILEY_H
