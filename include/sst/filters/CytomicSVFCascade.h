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

#ifndef INCLUDE_SST_FILTERS_CYTOMICSVFCASCADE_H
#define INCLUDE_SST_FILTERS_CYTOMICSVFCASCADE_H

#include <algorithm>

#include "detail/CytomicSVFCascadeDetails.h"

/*
 * Two critically damped Cytomic trapezoidal SVF cores (Simper,
 * SvfLinearTrapOptimised2) in series with one global resonance loop, giving a
 * 24 dB filter which resonates at its cutoff. Each core's lowpass is affine in its
 * input within a sample, so the loop has a closed form solution (or a Newton solve
 * when the loop is tanh driven).
 *
 * Every mode is a mix of the core outputs. Notch puts a zero pair on the cutoff,
 * Peak is that notch less twice the lowpass, and Allpass is exact at all resonances.
 * Compensated holds passband level as resonance rises.
 */

namespace sst::filters
{
template <cytomic_cascade::Mode M, bool Driven = false, bool Compensated = false>
struct CytomicSVFCascade
{
    using Coeff = cytomic_cascade::detail::Coeff;
    static constexpr int nCoeff = Coeff::count;

    SIMD_M128 ic1[2], ic2[2];
    SIMD_M128 C[nCoeff], dC[nCoeff];
    float srInv{1.f / 48000.f};
    bool firstBlock{true};

    CytomicSVFCascade()
    {
        init();
        for (int i = 0; i < nCoeff; ++i)
        {
            C[i] = SIMD_MM(setzero_ps)();
            dC[i] = SIMD_MM(setzero_ps)();
        }
    }

    void init()
    {
        for (int i = 0; i < 2; ++i)
        {
            ic1[i] = SIMD_MM(setzero_ps)();
            ic2[i] = SIMD_MM(setzero_ps)();
        }
    }

    void setCoeff(float freq, float res, float srInvIn)
    {
        srInv = srInvIn;
        float c[nCoeff];
        cytomic_cascade::detail::makeCoeffs<Driven>(freq, res, srInv, c);
        for (int i = 0; i < nCoeff; ++i)
            C[i] = SIMD_MM(set1_ps)(c[i]);
    }

    void setCoeff(float freqL, float freqR, float resL, float resR, float srInvIn)
    {
        srInv = srInvIn;
        float cL[nCoeff], cR[nCoeff];
        cytomic_cascade::detail::makeCoeffs<Driven>(freqL, resL, srInv, cL);
        cytomic_cascade::detail::makeCoeffs<Driven>(freqR, resR, srInv, cR);
        for (int i = 0; i < nCoeff; ++i)
            C[i] = SIMD_MM(set_ps)(0, 0, cR[i], cL[i]);
    }

    template <int blockSize> void setCoeffForBlock(float freq, float res, float srInvIn)
    {
        SIMD_M128 prior[nCoeff];
        std::copy(C, C + nCoeff, prior);
        setCoeff(freq, res, srInvIn);
        rampFrom<blockSize>(prior);
    }

    template <int blockSize>
    void setCoeffForBlock(float freqL, float freqR, float resL, float resR, float srInvIn)
    {
        SIMD_M128 prior[nCoeff];
        std::copy(C, C + nCoeff, prior);
        setCoeff(freqL, freqR, resL, resR, srInvIn);
        rampFrom<blockSize>(prior);
    }

    template <int blockSize> void retainCoeffForBlock()
    {
        for (int i = 0; i < nCoeff; ++i)
            dC[i] = SIMD_MM(setzero_ps)();
    }

    SIMD_M128 processBlockStepSSE(SIMD_M128 vin)
    {
        auto res = cytomic_cascade::detail::step<M, Driven, Compensated>(C, ic1, ic2, vin);
        cytomic_cascade::detail::advanceCoeffs(C, dC);
        return res;
    }

    void processBlockStep(float &L)
    {
        float r4 alignas(16)[4];
        SIMD_MM(store_ps)(r4, processBlockStepSSE(SIMD_MM(set_ps)(0, 0, 0, L)));
        L = r4[0];
    }

    void processBlockStep(float &L, float &R)
    {
        float r4 alignas(16)[4];
        SIMD_MM(store_ps)(r4, processBlockStepSSE(SIMD_MM(set_ps)(0, 0, R, L)));
        L = r4[0];
        R = r4[1];
    }

    template <int blockSize> void processBlock(const float *const inL, float *outL)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            outL[i] = inL[i];
            processBlockStep(outL[i]);
        }
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

    /*
     * Audio rate cutoff (filter fm). freq is the per sample cutoff in Hz;
     * resonance still comes from setCoeffForBlock and ramps across the block.
     */
    SIMD_M128 processBlockStepFMSSE(SIMD_M128 vin, SIMD_M128 freq)
    {
        cytomic_cascade::detail::rebuildCoeffsFromFreq(C, freq, srInv);
        auto res = cytomic_cascade::detail::step<M, Driven, Compensated>(C, ic1, ic2, vin);
        C[Coeff::r] = SIMD_MM(add_ps)(C[Coeff::r], dC[Coeff::r]);
        C[Coeff::gain] = SIMD_MM(add_ps)(C[Coeff::gain], dC[Coeff::gain]);
        return res;
    }

    template <int blockSize>
    void processBlockFM(const float *const inL, const float *const freqL, float *outL)
    {
        float r4 alignas(16)[4];
        for (int i = 0; i < blockSize; ++i)
        {
            SIMD_MM(store_ps)(r4, processBlockStepFMSSE(SIMD_MM(set_ps)(0, 0, 0, inL[i]),
                                                        SIMD_MM(set_ps)(0, 0, 0, freqL[i])));
            outL[i] = r4[0];
        }
    }

    template <int blockSize>
    void processBlockFM(const float *const inL, const float *const inR, const float *const freqL,
                        const float *const freqR, float *outL, float *outR)
    {
        float r4 alignas(16)[4];
        for (int i = 0; i < blockSize; ++i)
        {
            SIMD_MM(store_ps)(r4, processBlockStepFMSSE(SIMD_MM(set_ps)(0, 0, inR[i], inL[i]),
                                                        SIMD_MM(set_ps)(0, 0, freqR[i], freqL[i])));
            outL[i] = r4[0];
            outR[i] = r4[1];
        }
    }

  private:
    template <int blockSize> void rampFrom(SIMD_M128 *prior)
    {
        if (firstBlock)
        {
            std::copy(C, C + nCoeff, prior);
            firstBlock = false;
        }

        static constexpr float obsf = 1.f / blockSize;
        auto obs = SIMD_MM(set1_ps)(obsf);
        for (int i = 0; i < nCoeff; ++i)
        {
            dC[i] = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(C[i], prior[i]), obs);
            C[i] = prior[i];
        }
    }
};
} // namespace sst::filters

/*
 * The math
 * --------
 *
 * One core. Simper's trapezoidal SVF (SvfLinearTrapOptimised2) with
 * g = tan(pi fc / fs) and damping k is
 *
 *     a1 = 1 / (1 + g (g + k)),  a2 = g a1,  a3 = g a2
 *     v1 = a1 ic1 + a2 (v0 - ic2)          band
 *     v2 = ic2 + a2 ic1 + a3 (v0 - ic2)    low
 *     ic1 = 2 v1 - ic1,  ic2 = 2 v2 - ic2
 *
 * The trapezoidal rule is the bilinear map, so with the prewarped, normalised
 * s = (1/g) (1 - z^-1) / (1 + z^-1) the core is exactly LP = 1 / (s^2 + k s + 1)
 * and BP = s / (s^2 + k s + 1). We fix k = 2 (critically damped), so
 * LP = 1/(1+s)^2, and with q = g / (1+g) the coefficients are
 *
 *     a1 = (1-q)^2,  a2 = q (1-q),  a3 = q^2
 *
 * Affine lemma. Within a sample a core's low output is affine in its input:
 *
 *     v2 = a3 v0 + S,   S = a2 ic1 + (1 - a3) ic2
 *
 * and S depends only on state, so it is known before the input arrives.
 *
 * Cascade and loop. Two cores in series, a then b, with u the input to core a
 * and the loop u = x - r v2b. Chaining the lemma gives v2b = a3^2 u + St with
 * St = a3 Sa + Sb, so the loop solves exactly each sample as
 *
 *     u = (x - r St) / (1 + r a3^2) = (x - r St) inv,   inv = 1 / (1 + r q^4)
 *
 * The denominator is >= 1, so the solve is always well posed. Then the cores
 * run from u as usual. In s the closed loop is
 *
 *     u = x (1+s)^4 / D,   D = (1+s)^4 + r
 *
 * Resonance. At s = j, (1+j)^4 = -4, so D = r - 4: the loop resonates exactly
 * at fc (no stage cutoff offset is needed for two cores) and self-oscillates at
 * r = 4. The linear loop maps res 0..1 to r = 4 min(res, 0.99). The DC gain of
 * the lowpass is 1 / (1+r).
 *
 * Taps. In s the available signals are
 *
 *     v2a = u / (1+s)^2,  v2b = u / (1+s)^4,  v1a = s u / (1+s)^2,
 *     hp2 = v2a - 2 v1b - v2b = s^2 u / (1+s)^4
 *
 * and x = u D / (1+s)^4. Every mode is a fixed mix of these, so each mode is
 * (numerator) / D with
 *
 *     Lowpass   v2b                                   1
 *     Highpass  u - 4 v1a - v2b + 2 hp2               s^4
 *     Bandpass  4 hp2                                 4 s^2, unity at fc when r = 0
 *     Allpass   x - 8 v1a + 16 hp2                    (1-s)^4 + r
 *     Notch     x - (4-r)(hp2/2 + v1a/4)              zero pair at s = j
 *     Peak      notch - 2 (1+r) v2b                   as the 12 dB svf peak
 *
 * The highpass and allpass follow from expanding (1+s)^4 and ((1+s)^2 - 4s)^2.
 * For the notch, at s = j hp2/2 + v1a/4 = u/4 and x = (4-r) u/4, so the
 * output is zero there for every r. These identities
 * need k = 2 in every sample, which is why only q ramps across a block and a1,
 * a2, a3 are rebuilt from it each sample: ramping them independently leaves the
 * cores mid ramp off critical damping and a large cutoff jump then spikes the
 * non lowpass modes.
 *
 * Compensation. Multiplying by (1+r) restores unity lowpass gain at DC as
 * resonance rises.
 *
 * Driven. The loop becomes u = x - r tanh(v2b), so solve
 *
 *     f(u) = u + r tanh(a3^2 u + St) - x = 0
 *     f'(u) = 1 + r a3^2 (1 - tanh^2) >= 1
 *
 * f is monotone so the root is unique; two Newton steps seeded with the linear
 * solution are enough. Here res 0..1 maps to r = 4 * 1.25 res, so the top of the
 * range is past self-oscillation and settles to a bounded limit cycle at fc. The
 * compensation gain is applied to x ahead of the loop rather than to the output,
 * since on the output it would also scale the self-oscillation. Driven notch clamps
 * 4 - r at zero, so past r = 4 the notch simply closes.
 *
 * Why only two cores. With N cores the -180 degree point of 1/(1+s)^2N moves to
 * w = tan(pi / 2N) < 1 for N > 2, so the cores must sit above the resonant
 * cutoff. The 36 and 48 dB versions then cut less than this one for the first
 * octaves above fc (-7.5 and -5.5 dB at fc against -12, and less at 2 fc), and
 * only pull ahead past 4 to 8 fc. Steeper slopes are better served by the
 * Butterworth filters.
 */

#endif // INCLUDE_SST_FILTERS_CYTOMICSVFCASCADE_H
