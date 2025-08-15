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

#include "sst/filters++.h"
#include "catch2/catch2.hpp"
#include <iostream>

TEST_CASE("FiltersPlusPlus Ultra Basic")
{
    SECTION("Make a Vember LowPass")
    {
        namespace sfpp = sst::filtersplusplus;

        auto filter = sfpp::Filter();
        filter.setFilterModel(sfpp::FilterModels::VemberClassic);
        filter.setPassType(sfpp::PassTypes::LP);
        filter.setSlopeLevel(sfpp::SlopeLevels::Slope_24db);
        filter.setDriveType(sfpp::DriveTypes::Standard);
        REQUIRE(filter.prepareInstance());
        REQUIRE(filter.coefficientsExtraCount(filter.getFilterModel(),
                                              filter.getModelConfiguration()) == 0);
    }

    SECTION("Vember AllPass isn't a thing")
    {
        namespace sfpp = sst::filtersplusplus;

        auto filter = sfpp::Filter();
        filter.setFilterModel(sfpp::FilterModels::VemberClassic);
        filter.setPassType(sfpp::PassTypes::AllPass);
        filter.setSlopeLevel(sfpp::SlopeLevels::Slope_24db);
        filter.setDriveType(sfpp::DriveTypes::Standard);
        REQUIRE_FALSE(filter.prepareInstance());
    }

    SECTION("OBXD 4Pole")
    {
        namespace sfpp = sst::filtersplusplus;
        auto filter = sfpp::Filter();

        filter.setSampleRateAndBlockSize(48000, 16);

        filter.setFilterModel(sfpp::FilterModels::OBXD_4Pole);
        filter.setPassType(sfpp::PassTypes::LP);
        filter.setSlopeLevel(sfpp::SlopeLevels::Slope_18db);
        if (!filter.prepareInstance())
            REQUIRE(false);

        size_t blockPos{0};
        for (int i = 0; i < 100; ++i)
        {
            if (blockPos == 0)
            {
                for (int v = 0; v < 4; ++v)
                {
                    filter.makeCoefficients(v, -9, 0.1 + v * 0.2);
                }
                filter.prepareBlock();
            }

            auto out = filter.processSample(SIMD_MM(setzero_ps)());

            blockPos++;
            if (blockPos == filter.getBlockSize())
            {
                filter.concludeBlock();
                blockPos = 0;
            }
        }
    }
}

TEST_CASE("FiltersPlusPlus API Consistency")
{
    SECTION("Mono Stereo Quad and SIMD all align")
    {
        auto mkf = []() {
            namespace sfpp = sst::filtersplusplus;

            auto filter = sfpp::Filter();
            filter.setFilterModel(sfpp::FilterModels::VemberClassic);
            filter.setPassType(sfpp::PassTypes::LP);
            filter.setSlopeLevel(sfpp::SlopeLevels::Slope_24db);
            filter.setDriveType(sfpp::DriveTypes::Standard);

            filter.setSampleRateAndBlockSize(48000, 16);
            REQUIRE(filter.prepareInstance());
            return filter;
        };

        auto f1 = mkf();
        auto f2 = mkf();
        f2.setMono();
        auto f3 = mkf();
        f3.setStereo();
        auto f4 = mkf();
        f4.setQuad();

        int blockPos{0};
        float ph{0.f};
        float dph{440.0 / 48000.0};
        for (int i = 0; i < 1000; ++i)
        {
            if (blockPos == 0)
            {
                for (int v = 0; v < 4; ++v)
                {
                    f1.makeCoefficients(v, -9, 0.1 + v * 0.2);
                    f2.makeCoefficients(v, -9, 0.1 + v * 0.2);
                    f3.makeCoefficients(v, -9, 0.1 + v * 0.2);
                    f4.makeCoefficients(v, -9, 0.1 + v * 0.2);
                }
                f1.prepareBlock();
                f2.prepareBlock();
                f3.prepareBlock();
                f4.prepareBlock();
            }

            auto sv = std::sin(ph);
            auto cv = std::cos(ph);
            ph += dph;
            if (ph > 1)
                ph -= 1;

            INFO("Iteration " << i << " ph=" << ph << " sv=" << sv << " cv=" << cv);

            // we want order sin cos -sin -cos across the quad but remember that set_ps is backwards
            auto outSimd = f1.processSample(SIMD_MM(set_ps)(-cv, -sv, cv, sv));
            float ov alignas(16)[4];
            SIMD_MM(store_ps)(ov, outSimd);

            auto outMono = f2.processMonoSample(sv);
            REQUIRE(outMono == ov[0]);

            float oL, oR;
            f3.processStereoSample(sv, cv, oL, oR);
            REQUIRE(oL == ov[0]);
            REQUIRE(oR == ov[1]);

            float inp[4]{sv, cv, -sv, -cv};
            float outp[4]{0.f, 0.f, 0.f, 0.f};
            f4.processQuadSample(inp, outp);
            REQUIRE(outp[0] == ov[0]);
            REQUIRE(outp[1] == ov[1]);
            REQUIRE(outp[2] == ov[2]);
            REQUIRE(outp[3] == ov[3]);

            blockPos++;
            if (blockPos == f1.getBlockSize())
            {
                f1.concludeBlock();
                f2.concludeBlock();
                f3.concludeBlock();
                f4.concludeBlock();
            }
        }
    }

    SECTION("constant coefficients work")
    {
        auto mkf = []() {
            namespace sfpp = sst::filtersplusplus;

            auto filter = sfpp::Filter();
            filter.setFilterModel(sfpp::FilterModels::VemberClassic);
            filter.setPassType(sfpp::PassTypes::LP);
            filter.setSlopeLevel(sfpp::SlopeLevels::Slope_24db);
            filter.setDriveType(sfpp::DriveTypes::Standard);

            filter.setSampleRateAndBlockSize(48000, 16);
            REQUIRE(filter.prepareInstance());
            return filter;
        };

        auto f1 = mkf();
        auto f2 = mkf();
        double ph{0};
        auto dph = 440.0 / 48000.0;
        size_t blockPos{0};
        for (int i = 0; i < 1000; ++i)
        {
            if (blockPos == 0)
            {
                for (int v = 0; v < 4; ++v)
                {
                    f1.makeCoefficients(v, -9, 0.1 + v * 0.2);
                    if (i == 0)
                    {
                        f2.makeConstantCoefficients(v, -9, 0.1 + v * 0.2);
                    }
                }
                f1.prepareBlock();
                f2.prepareBlock();
            }

            auto sv = std::sin(ph);
            auto cv = std::cos(ph);
            ph += dph;
            if (ph > 1)
                ph -= 1;

            INFO("Iteration " << i << " ph=" << ph << " sv=" << sv << " cv=" << cv);

            // we want order sin cos -sin -cos across the quad but remember that set_ps is backwards
            auto outOne = f1.processSample(SIMD_MM(set_ps)(-cv, -sv, cv, sv));
            auto outTwo = f2.processSample(SIMD_MM(set_ps)(-cv, -sv, cv, sv));

            REQUIRE(memcmp(&outOne, &outTwo, sizeof(SIMD_M128)) == 0);
        }
    }
}
