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

TEST_CASE("Filters++ Ultra Basic")
{
    SECTION("Make a Vember Lowpass")
    {
        namespace sfpp = sst::filtersplusplus;

        auto filter = sfpp::Filter();
        filter.setFilterModel(sfpp::FilterModel::VemberClassic);
        filter.setPassband(sfpp::Passband::LP);
        filter.setSlope(sfpp::Slope::Slope_24dB);
        filter.setDriveMode(sfpp::DriveMode::Standard);
        REQUIRE(filter.prepareInstance());
        REQUIRE(filter.coefficientsExtraCount(filter.getFilterModel(),
                                              filter.getModelConfiguration()) == 0);
    }

    SECTION("Vember Allpass isn't a thing")
    {
        namespace sfpp = sst::filtersplusplus;

        auto filter = sfpp::Filter();
        filter.setFilterModel(sfpp::FilterModel::VemberClassic);
        filter.setPassband(sfpp::Passband::Allpass);
        filter.setSlope(sfpp::Slope::Slope_24dB);
        filter.setDriveMode(sfpp::DriveMode::Standard);
        REQUIRE_FALSE(filter.prepareInstance());
    }

    SECTION("OB-Xd 4-pole")
    {
        namespace sfpp = sst::filtersplusplus;
        auto filter = sfpp::Filter();

        filter.setSampleRateAndBlockSize(48000, 16);

        filter.setFilterModel(sfpp::FilterModel::OBXD_4Pole);
        filter.setPassband(sfpp::Passband::LP);
        filter.setSlope(sfpp::Slope::Slope_18dB);
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
            filter.setFilterModel(sfpp::FilterModel::VemberClassic);
            filter.setPassband(sfpp::Passband::LP);
            filter.setSlope(sfpp::Slope::Slope_24dB);
            filter.setDriveMode(sfpp::DriveMode::Standard);

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

    SECTION("Constant coefficients give same result as recalculating")
    {
        int BS = 16;
        auto mkf = []() {
            namespace sfpp = sst::filtersplusplus;

            auto filter = sfpp::Filter();
            filter.setFilterModel(sfpp::FilterModel::VemberClassic);
            filter.setPassband(sfpp::Passband::LP);
            filter.setSlope(sfpp::Slope::Slope_24dB);
            filter.setDriveMode(sfpp::DriveMode::Standard);

            filter.setSampleRateAndBlockSize(48000, BS);
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

    SECTION("Frozen coefficients same as computing every block, including on change")
    {
        int BS = 16;
        auto mkf = []() {
            namespace sfpp = sst::filtersplusplus;

            auto filter = sfpp::Filter();
            filter.setFilterModel(sfpp::FilterModel::VemberClassic);
            filter.setPassband(sfpp::Passband::LP);
            filter.setSlope(sfpp::Slope::Slope_24dB);
            filter.setDriveMode(sfpp::DriveMode::Standard);

            filter.setSampleRateAndBlockSize(48000, BS);
            REQUIRE(filter.prepareInstance());
            return filter;
        };

        auto f1 = mkf();
        auto f2 = mkf();
        double ph{0};
        auto dph = 440.0 / 48000.0;

        for (int i = 0; i < 1000; ++i)
        {
            // increase an octave after half the samples
            auto pitch = (std::floor(i / 500) * 12);

            if (i % BS == 0)
            {
                f1.makeCoefficients(0, pitch, 0.5f);
                if (i == 0)
                {
                    f2.makeCoefficients(0, pitch, 0.5f);
                }
                else
                {
                    f2.freezeCoefficients(0);
                }
                f1.prepareBlock();
                f2.prepareBlock();
            }

            ph += dph;
            if (ph > 1)
                ph -= 1;

            INFO("Iteration " << i << " ph=" << ph);

            auto saw = ph * 2 - 1;
            auto outOne = f1.processMonoSample(saw);
            auto outTwo = f2.processMonoSample(saw);

            REQUIRE(memcmp(&outOne, &outTwo, sizeof(float)) == 0);

            if (i % BS == BS - 1)
            {
                f1.concludeBlock();
                f2.concludeBlock();
            }
        }
    }
}

TEST_CASE("Configuration Selector")
{
    namespace sfpp = sst::filtersplusplus;

    SECTION("sfpp::get<enum> works")
    {
        auto cf = sfpp::Filter::availableModelConfigurations(sfpp::FilterModel::CutoffWarp);
        auto m = cf[0];
        REQUIRE(sfpp::get<sfpp::Passband>(m) == m.pt);
        REQUIRE(sfpp::get<sfpp::Slope>(m) == m.st);
        REQUIRE(sfpp::get<sfpp::DriveMode>(m) == m.dt);
        REQUIRE(sfpp::get<sfpp::FilterSubModel>(m) == m.mt);
    }

    SECTION("Get passbands for a few things")
    {
        auto pbv = sfpp::potentialValuesFor<sfpp::Passband>(sfpp::FilterModel::VemberClassic);
        REQUIRE(pbv.size() == 5);

        auto sbv = sfpp::potentialValuesFor<sfpp::Slope>(sfpp::FilterModel::VemberClassic);
        REQUIRE(sbv.size() == 2);

        auto sobv = sfpp::potentialValuesFor<sfpp::Slope>(sfpp::FilterModel::OBXD_4Pole);
        REQUIRE(sobv.size() == 5);
    }

    SECTION("Model is valid (full)")
    {
        auto fm = sfpp::FilterModel::CutoffWarp;
        auto cf = sfpp::Filter::availableModelConfigurations(fm);
        for (auto c : cf)
            REQUIRE(sfpp::isModelConfigValid(fm, c));

        for (auto c : cf)
        {
            c.pt = sst::filtersplusplus::Passband::NotchAndLP;
            REQUIRE(!sfpp::isModelConfigValid(fm, c));
        }
    }

    SECTION("Model is valid (partial)")
    {
        auto fm = sfpp::FilterModel::CutoffWarp;
        auto cf = sfpp::Filter::availableModelConfigurations(fm);
        auto tmc = cf[0];
        REQUIRE(sfpp::isPartialConfigValid(fm, sfpp::Passband::LP));
        REQUIRE(!sfpp::isPartialConfigValid(fm, sfpp::Passband::LowShelf));

        REQUIRE(sfpp::isPartialConfigValid(fm, sfpp::Passband::LP, sfpp::Slope::UNSUPPORTED));
        REQUIRE(
            !sfpp::isPartialConfigValid(fm, sfpp::Passband::LowShelf, sfpp::Slope::UNSUPPORTED));
        REQUIRE(!sfpp::isPartialConfigValid(fm, sfpp::Passband::LP, sfpp::Slope::Slope_6dB));

        REQUIRE(sfpp::isPartialConfigValid(fm, sfpp::Passband::LP, sfpp::Slope::UNSUPPORTED,
                                           sfpp::DriveMode::OJD));
        REQUIRE(!sfpp::isPartialConfigValid(fm, sfpp::Passband::LowShelf, sfpp::Slope::UNSUPPORTED,
                                            sfpp::DriveMode::OJD));
        REQUIRE(!sfpp::isPartialConfigValid(fm, sfpp::Passband::LP, sfpp::Slope::UNSUPPORTED,
                                            sfpp::DriveMode::K35_Continuous));
        REQUIRE(!sfpp::isPartialConfigValid(fm, sfpp::Passband::LP, sfpp::Slope::Slope_6dB,
                                            sfpp::DriveMode::K35_Continuous));
    }

    SECTION("Partial Configs with nothing configed")
    {
        auto fm = sfpp::FilterModel::VemberClassic;

        REQUIRE(sfpp::valuesAndValidityForPartialConfig<sfpp::Passband>(fm).size() ==
                sfpp::potentialValuesFor<sfpp::Passband>(fm).size());

        // OK so lets compare LP and Allpass
        auto lpSlpes = sfpp::valuesAndValidityForPartialConfig<sfpp::Slope>(fm, sfpp::Passband::LP);
        auto apSlpes =
            sfpp::valuesAndValidityForPartialConfig<sfpp::Slope>(fm, sfpp::Passband::Allpass);

        for (auto [l, v] : lpSlpes)
        {
            std::cout << sfpp::toString(l) << " " << v << std::endl;
        }

        for (auto [l, v] : apSlpes)
        {
            std::cout << sfpp::toString(l) << " " << v << std::endl;
        }
    }
}