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

#ifndef INCLUDE_SST_FILTERS_BUTTERWORTHLPHP_H
#define INCLUDE_SST_FILTERS_BUTTERWORTHLPHP_H

#include <cmath>
#include <array>
#include <numbers>

namespace sst::filters
{

enum class ButterworthType
{
    Lowpass,
    Highpass
};

/**
 * Cascaded TPT state-variable Butterworth filter, even orders only.
 * N is the filter order (poles); attenuation is N*6 dB/oct.
 */
template <int N, ButterworthType Type> class Butterworth
{
    static_assert(N >= 2 && N % 2 == 0, "N must be a positive even integer");
    static constexpr int kStages = N / 2;
    static constexpr bool kIsHP = (Type == ButterworthType::Highpass);

  public:
    Butterworth() = default;

    void setCutoffAndSampleRate(float cutoffHz, float sampleRate)
    {
        const float fcMax = 0.49f * sampleRate;
        if (cutoffHz > fcMax)
            cutoffHz = fcMax;
        if (cutoffHz < 1.0f)
            cutoffHz = 1.0f;

        const float g = std::tan(std::numbers::pi_v<float> * cutoffHz / sampleRate);
        gCoeff_ = g;

        for (int i = 0; i < kStages; ++i)
        {
            a_[i] = 1.0f / (1.0f + g * (g + kQ_[i]));
            kPlusG_[i] = kQ_[i] + g;
        }
    }

    // ---- Mono ----

    inline float processSample(float x)
    {
        float sig = x;
        for (int i = 0; i < kStages; ++i)
        {
            const float a = a_[i];
            const float kpg = kPlusG_[i];
            const float g = gCoeff_;

            const float hp = (sig - kpg * z1_[i] - z2_[i]) * a;
            const float bp = g * hp + z1_[i];
            const float lp = g * bp + z2_[i];
            z1_[i] = g * hp + bp;
            z2_[i] = g * bp + lp;

            if constexpr (kIsHP)
                sig = hp;
            else
                sig = lp;
        }
        return sig;
    }

    void processBlock(float *data, int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
            data[n] = processSample(data[n]);
    }

    void processBlock(const float *in, float *out, int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
            out[n] = processSample(in[n]);
    }

    // ---- Stereo ----

    inline void processSample(float &l, float &r)
    {
        float sigL = l, sigR = r;
        for (int i = 0; i < kStages; ++i)
        {
            const float a = a_[i];
            const float kpg = kPlusG_[i];
            const float g = gCoeff_;

            const float hpL = (sigL - kpg * z1L_[i] - z2L_[i]) * a;
            const float hpR = (sigR - kpg * z1R_[i] - z2R_[i]) * a;

            const float bpL = g * hpL + z1L_[i];
            const float bpR = g * hpR + z1R_[i];

            const float lpL = g * bpL + z2L_[i];
            const float lpR = g * bpR + z2R_[i];

            z1L_[i] = g * hpL + bpL;
            z1R_[i] = g * hpR + bpR;
            z2L_[i] = g * bpL + lpL;
            z2R_[i] = g * bpR + lpR;

            if constexpr (kIsHP)
            {
                sigL = hpL;
                sigR = hpR;
            }
            else
            {
                sigL = lpL;
                sigR = lpR;
            }
        }
        l = sigL;
        r = sigR;
    }

    void processBlock(float *left, float *right, int numSamples)
    {
        for (int n = 0; n < numSamples; ++n)
            processSample(left[n], right[n]);
    }

    // ---- Lifecycle ----

    void reset()
    {
        for (int i = 0; i < kStages; ++i)
        {
            z1_[i] = z2_[i] = 0.0f;
            z1L_[i] = z2L_[i] = 0.0f;
            z1R_[i] = z2R_[i] = 0.0f;
        }
    }

  private:
    static std::array<float, kStages> computeQ()
    {
        std::array<float, kStages> q{};
        for (int i = 0; i < kStages; ++i)
        {
            const double theta = std::numbers::pi * (2.0 * i + 1.0) / (2.0 * N);
            q[i] = static_cast<float>(2.0 * std::cos(theta));
        }
        return q;
    }

    const std::array<float, kStages> kQ_ = computeQ();

    std::array<float, kStages> a_{}, kPlusG_{};
    float gCoeff_ = 0.0f;

    std::array<float, kStages> z1_{}, z2_{};
    std::array<float, kStages> z1L_{}, z2L_{}, z1R_{}, z2R_{};
};

// Convenience aliases
template <int N> using ButterworthLP = Butterworth<N, ButterworthType::Lowpass>;
template <int N> using ButterworthHP = Butterworth<N, ButterworthType::Highpass>;

} // namespace sst::filters

#endif // INCLUDE_SST_FILTERS_BUTTERWORTHLPHP_H