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

#include "catch2/catch2.hpp"
#include "sst/filters/CytomicSVF.h"

#include <iostream>
#include <cmath>
#include <utility>
#include <vector>
#include <fstream>

std::vector<std::pair<float, float>> rmsCurve(int mode, double cutoffFreq, double bellAmp = 0.f)
{
    double sr{48000}, sri{1.0 / sr};

    sst::filters::CytomicSVF cy;

    std::vector<std::pair<float, float>> res;

    for (int key = 0; key < 128; ++key)
    {
        auto freq = 440.0 * pow(2.0, (key - 69.0) / 12);

        cy.setCoeff((sst::filters::CytomicSVF::Mode)mode, cutoffFreq, 0.7, sri, bellAmp);
        cy.init();

        float rmsIn{0}, rmsOut{0};
        int npoints = sr * 0.1;
        for (int i = 0; i < npoints; ++i)
        {
            auto t = i * sri;
            float sinv = sin(2 * M_PI * t * freq);
            float s = sinv;
            sst::filters::CytomicSVF::step(cy, s, s);
            rmsIn += sinv * sinv;
            rmsOut += s * s;
        }
        rmsIn = sqrt(rmsIn / npoints);
        rmsOut = sqrt(rmsOut / npoints);
        res.emplace_back(freq, rmsOut / rmsIn);
    }
    return res;
}

TEST_CASE("Cytomic SVF")
{
    SECTION("Low Pass")
    {
        auto curve = rmsCurve(sst::filters::CytomicSVF::Mode::LP, 440.0);
        float lastRMS = curve[0].second;
        for (auto [f, out] : curve)
        {
            INFO(f);

            if (f >= 440.0)
            {
                // decreasing beyond peak
                REQUIRE(lastRMS >= out);
            }
            lastRMS = out;
        }
    }

    SECTION("High Pass")
    {
        auto curve = rmsCurve(sst::filters::CytomicSVF::Mode::HP, 440.0);
        float lastRMS = curve[0].second;
        for (auto [f, out] : curve)
        {
            INFO(f);

            if (f <= 440.0)
            {
                // decreasing beyond peak
                REQUIRE(lastRMS <= out);
            }
            lastRMS = out;
        }
    }
}

TEST_CASE("Cytomic SVF Response Curves")
{
    REQUIRE(true);
    static constexpr int nOut{6}, nPts{1024};
    float freq[nPts], out[nOut][nPts];

    /* Frequency Sweep */
    auto mode = sst::filters::CytomicSVF::Mode::HIGH_SHELF;
    float fr = 220;
    for (int idx = 0; idx < nOut; ++idx)
    {
        sst::filters::CytomicSVFGainProfile(mode, fr, 0.2, 0.3, freq, out[idx], nPts);
        fr = fr * 2;
    }

    std::ofstream outFile("/tmp/cy.csv");
    if (outFile.is_open())
    {
        for (auto i = 0U; i < nPts; ++i)
        {
            outFile << freq[i];
            for (auto j = 0U; j < nOut; ++j)
            {
                outFile << ", " << out[j][i];
            }
            outFile << "\n";
        }
        outFile.close();
    }
}