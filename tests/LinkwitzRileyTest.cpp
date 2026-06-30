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

#include "catch2/catch2.hpp"
#include "sst/filters/LinkwitzRiley.h"

#include <cmath>

namespace
{
constexpr double sr{48000}, sri{1.0 / sr};

struct Bands
{
    double in, low, high, sum; // RMS of input, low band, high band, and (low + high)
};

/*
 * Drive the crossover with a stereo tone (same signal both channels) and return
 * the steady-state RMS of the input, the two bands, and their sum. We warm up
 * for 50 ms then integrate over a whole number of cycles so the RMS of a pure
 * tone is exact, which keeps the reconstruction check tight.
 */
Bands measure(float crossoverFreq, float toneFreq)
{
    sst::filters::LinkwitzRileyLR4Crossover lr;
    lr.setCoeff(crossoverFreq, sri);
    lr.init();

    int warm = (int)(0.05 * sr);
    int cycles = 80;
    int meas = (int)std::round(cycles * sr / toneFreq);

    double inSq{0}, lowSq{0}, highSq{0}, sumSq{0};
    for (int i = 0; i < warm + meas; ++i)
    {
        auto t = i * sri;
        float s = (float)std::sin(2 * M_PI * toneFreq * t);
        float lL, lR, hL, hR;
        lr.step(s, s, lL, lR, hL, hR);

        if (i >= warm)
        {
            float sum = lL + hL;
            inSq += (double)s * s;
            lowSq += (double)lL * lL;
            highSq += (double)hL * hL;
            sumSq += (double)sum * sum;
        }
    }
    return {std::sqrt(inSq / meas), std::sqrt(lowSq / meas), std::sqrt(highSq / meas),
            std::sqrt(sumSq / meas)};
}
} // namespace

TEST_CASE("Linkwitz-Riley LR4 Crossover")
{
    float xover = 1000.f;

    SECTION("Bands sum to a flat (allpass) magnitude at every frequency")
    {
        // The defining LR4 property: the two bands are in phase everywhere, so
        // low + high reconstructs the input with no boost or notch. A flat
        // magnitude sum across the spectrum is exactly the "constant phase"
        // (phase-coherent) behaviour we are after.
        for (float f : {20.f, 40.f, 80.f, 160.f, 320.f, 640.f, 1000.f, 1280.f, 2560.f, 5120.f,
                        10240.f, 18000.f})
        {
            auto b = measure(xover, f);
            INFO("tone " << f << " Hz: in=" << b.in << " sum=" << b.sum
                         << " ratio=" << b.sum / b.in);
            REQUIRE(b.sum / b.in == Approx(1.0).margin(0.01));
        }
    }

    SECTION("Each band is -6 dB at the crossover")
    {
        auto b = measure(xover, xover);
        REQUIRE(b.low / b.in == Approx(0.5).margin(0.02));
        REQUIRE(b.high / b.in == Approx(0.5).margin(0.02));
    }

    SECTION("Low band passes lows and blocks highs; high band the reverse")
    {
        auto below = measure(xover, xover / 10); // 100 Hz, a decade below
        REQUIRE(below.low / below.in > 0.95f);
        REQUIRE(below.high / below.in < 0.05f);

        auto above = measure(xover, xover * 10); // 10 kHz, a decade above
        REQUIRE(above.high / above.in > 0.95f);
        REQUIRE(above.low / above.in < 0.05f);
    }
}

TEST_CASE("Linkwitz-Riley reconstructs each stereo channel independently")
{
    // Different tone per channel proves the [L, R, L, R] lane packing keeps the
    // two channels separate while still reconstructing each one.
    sst::filters::LinkwitzRileyLR4Crossover lr;
    lr.setCoeff(800.f, sri);
    lr.init();

    float fL = 200.f, fR = 4000.f;
    int n = (int)(0.3 * sr);
    int warm = (int)(0.05 * sr);

    double errL{0}, errR{0}, refL{0}, refR{0};
    for (int i = 0; i < n; ++i)
    {
        auto t = i * sri;
        float l = (float)std::sin(2 * M_PI * fL * t);
        float r = (float)std::sin(2 * M_PI * fR * t);
        float lL, lR, hL, hR;
        lr.step(l, r, lL, lR, hL, hR);

        if (i >= warm)
        {
            // each channel's bands recombine to that channel only
            errL += (double)(lL + hL) * (lL + hL);
            errR += (double)(lR + hR) * (lR + hR);
            refL += (double)l * l;
            refR += (double)r * r;
        }
    }
    // reconstructed energy matches the original per-channel energy (allpass)
    REQUIRE(std::sqrt(errL / refL) == Approx(1.0).margin(0.01));
    REQUIRE(std::sqrt(errR / refR) == Approx(1.0).margin(0.01));
}

TEST_CASE("Linkwitz-Riley block processing matches per-sample stepping")
{
    // At a constant cutoff the smoothing deltas are zero, so the block path must
    // reproduce the per-sample step path exactly.
    static constexpr int bs = 64;
    float xover = 1200.f;

    sst::filters::LinkwitzRileyLR4Crossover perSample, block;
    perSample.setCoeff(xover, sri);
    perSample.init();
    block.init();

    float inL[bs], inR[bs];
    float blL[bs], blR[bs], bhL[bs], bhR[bs];

    for (int blk = 0; blk < 8; ++blk)
    {
        for (int i = 0; i < bs; ++i)
        {
            auto t = (blk * bs + i) * sri;
            inL[i] = (float)std::sin(2 * M_PI * 300.f * t);
            inR[i] = (float)std::sin(2 * M_PI * 2500.f * t);
        }

        block.setCoeffForBlock<bs>(xover, sri);
        block.processBlock<bs>(inL, inR, blL, blR, bhL, bhR);

        for (int i = 0; i < bs; ++i)
        {
            float lL, lR, hL, hR;
            perSample.step(inL[i], inR[i], lL, lR, hL, hR);
            REQUIRE(blL[i] == Approx(lL));
            REQUIRE(blR[i] == Approx(lR));
            REQUIRE(bhL[i] == Approx(hL));
            REQUIRE(bhR[i] == Approx(hR));
        }
    }
}
