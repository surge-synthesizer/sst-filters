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
#include "sst/filters/CytomicSVF.h"
#include "TestUtils.h"

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
    SECTION("Lowpass")
    {
        auto curve = rmsCurve((int)sst::filters::CytomicSVF::Mode::Lowpass, 440.0);
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

    SECTION("Highpass")
    {
        auto curve = rmsCurve((int)sst::filters::CytomicSVF::Mode::Highpass, 440.0);
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
#define MAKE_GNUPLOT 0

    for (int modei = (int)sst::filters::CytomicSVF::Mode::Lowpass;
         modei <= (int)sst::filters::CytomicSVF::Mode::HighShelf; ++modei)
    {
        auto mode = (sst::filters::CytomicSVF::Mode)modei;

        for (auto res = 0.0; res <= 1.0; res += 0.25)
        {
            DYNAMIC_SECTION("Mode: " << (int)mode << ", Res: " << res)
            {
                res = std::clamp(res, 0., 0.999999);

                static constexpr int nOut{6}, nPts{128};
                float freq[nPts], out[nOut][nPts], outBrute[nOut][nPts];

                /* Frequency Sweep */
                float fr = 220;
                auto res = 0.4;
                auto amp = 0.8;
                for (int idx = 0; idx < nOut; ++idx)
                {
                    float diff{0.f};
                    sst::filters::CytomicSVFGainProfile(mode, fr, res, amp, freq, out[idx], nPts);

                    // brute force it

                    for (int i = 0; i < nPts; ++i)
                    {
                        auto sr = 48000;
                        auto sri = 1.0 / sr;
                        sst::filters::CytomicSVF svf;
                        svf.setCoeff(mode, fr, res, sri, amp);
                        auto s = 2.0 * M_PI * freq[i] * sri;
                        auto t = 0.;
                        for (int j = 0; j < 100; ++j)
                        {
                            float v = sin(t);
                            auto r = v;
                            t += s;
                            sst::filters::CytomicSVF::step(svf, v, r);
                        }

                        double irms{0}, orms{0};
                        for (int j = 0; j < 1000; ++j)
                        {
                            float v = sin(t);
                            auto r = v;
                            t += s;
                            irms += v * v;
                            sst::filters::CytomicSVF::step(svf, v, r);
                            orms += v * v;
                        }
                        outBrute[idx][i] = std::sqrt(orms / irms);
                        diff += std::abs(out[idx][i] - outBrute[idx][i]);
                    }

                    if (mode == sst::filters::CytomicSVF::Mode::LowShelf ||
                        mode == sst::filters::CytomicSVF::Mode::HighShelf)
                    {
                        // Thr shelf is a bit frequency nudged
                        REQUIRE(diff / nPts < 0.02);
                    }
                    else
                    {
                        REQUIRE(diff / nPts < 0.0025);
                    }

                    fr = fr * 2;
                }
#if MAKE_GNUPLOT
                std::ofstream outFile("/tmp/cy.csv");
                if (outFile.is_open())
                {
                    for (auto i = 0U; i < nPts; ++i)
                    {
                        outFile << freq[i];
                        for (auto j = 0U; j < nOut; ++j)
                        {
                            outFile << ", " << out[j][i] << ", " << outBrute[j][i];
                        }
                        outFile << "\n";
                    }
                    outFile.close();
                }
#endif
            }
        }
    }
}

TEST_CASE("Cytomic SVF Quad Filter Test")
{
    using namespace TestUtils;
    namespace sfpp = sst::filtersplusplus;

    std::vector<RMSSet> ans = {
        {-2.81053f, -2.27167f, -3.09009f, -16.3563f, -51.5564f},
        {-31.4792f, -15.8393f, -3.12771f, -2.2901f, -3.00617f},
        {-17.8248f, -9.17934f, -3.11892f, -9.39847f, -31.4778f},
        {-3.10104f, -4.26682f, -23.7251f, -4.15931f, -3.01845f},
        {-2.52717f, -0.639164f, 2.90231f, -0.742297f, -2.99382f},
        {-2.99243f, -3.0779f, -3.15685f, -3.02565f, -3.01241f},
    };
    runTest({FilterType::fut_cytomic_svf, FilterSubType::st_cytomic_lp, ans[0]});
    runTest(sfpp::FilterModel::CytomicSVF, sfpp::Passband::LP, 0, 0.5, ans[0]);

    runTest({FilterType::fut_cytomic_svf, FilterSubType::st_cytomic_hp, ans[1]});
    runTest(sfpp::FilterModel::CytomicSVF, sfpp::Passband::HP, 0, 0.5, ans[1]);

    runTest({FilterType::fut_cytomic_svf, FilterSubType::st_cytomic_bp, ans[2]});
    runTest(sfpp::FilterModel::CytomicSVF, sfpp::Passband::BP, 0, 0.5, ans[2]);

    runTest({FilterType::fut_cytomic_svf, FilterSubType::st_cytomic_notch, ans[3]});
    runTest(sfpp::FilterModel::CytomicSVF, sfpp::Passband::Notch, 0, 0.5, ans[3]);

    runTest({FilterType::fut_cytomic_svf, FilterSubType::st_cytomic_peak, ans[4]});
    runTest(sfpp::FilterModel::CytomicSVF, sfpp::Passband::Peak, 0, 0.5, ans[4]);

    runTest({FilterType::fut_cytomic_svf, FilterSubType::st_cytomic_allpass, ans[5]});
    runTest(sfpp::FilterModel::CytomicSVF, sfpp::Passband::Allpass, 0, 0.5, ans[5]);
}
