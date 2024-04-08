//
// Created by Paul Walker on 4/7/24.
//

#include "catch2/catch2.hpp"
#include "sst/filters/CytomicSVF.h"

#include <iostream>
#include <cmath>
#include <utility>
#include <vector>

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