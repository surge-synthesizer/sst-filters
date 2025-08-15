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
#include "TestUtils.h"

TEST_CASE("OBXD Filter")
{
    using namespace TestUtils;
    namespace sfpp = sst::filtersplusplus;

    SECTION("2-pole LPF")
    {
        runTest({FilterType::fut_obxd_2pole_lp,
                 static_cast<FilterSubType>(0),
                 {-5.42604f, -4.8885f, -5.72345f, -18.9727f, -54.1739f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::LP, sfpp::DriveTypes::Standard},
                0, 0.5, {-5.42604f, -4.8885f, -5.72345f, -18.9727f, -54.1739f});

        runTest({FilterType::fut_obxd_2pole_lp,
                 static_cast<FilterSubType>(1),
                 {-5.40362f, -4.74114f, -5.10849f, -18.8247f, -53.902f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::LP, sfpp::DriveTypes::Pushed}, 0,
                0.5, {-5.40362f, -4.74114f, -5.10849f, -18.8247f, -53.902f});
    }

    SECTION("2-pole BPF")
    {
        runTest({FilterType::fut_obxd_2pole_bp,
                 static_cast<FilterSubType>(0),
                 {-26.4609f, -17.8167f, -11.7726f, -18.0352f, -40.1138f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::BP, sfpp::DriveTypes::Standard},
                0, 0.5, {-26.4609f, -17.8167f, -11.7726f, -18.0352f, -40.1138f});

        runTest({FilterType::fut_obxd_2pole_bp,
                 static_cast<FilterSubType>(1),
                 {-26.4323f, -17.6603f, -11.1551f, -17.8948f, -40.1103f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::BP, sfpp::DriveTypes::Pushed}, 0,
                0.5, {-26.4323f, -17.6603f, -11.1551f, -17.8948f, -40.1103f});
    }

    SECTION("2-pole HPF")
    {
        runTest({FilterType::fut_obxd_2pole_hp,
                 static_cast<FilterSubType>(0),
                 {-34.0961f, -18.4561f, -5.7608f, -4.90633f, -5.62153f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::HP, sfpp::DriveTypes::Standard},
                0, 0.5, {-34.0961f, -18.4561f, -5.7608f, -4.90633f, -5.62153f});

        runTest({FilterType::fut_obxd_2pole_hp,
                 static_cast<FilterSubType>(1),
                 {-34.0065f, -18.2937f, -5.14328f, -4.76908f, -5.62067f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::HP, sfpp::DriveTypes::Pushed}, 0,
                0.5, {-34.0065f, -18.2937f, -5.14328f, -4.76908f, -5.62067f});
    }

    SECTION("2-pole Notch")
    {
        runTest({FilterType::fut_obxd_2pole_n,
                 static_cast<FilterSubType>(0),
                 {-11.7372f, -12.9044f, -32.3445f, -12.7962f, -11.6544f}});
        runTest(sfpp::FilterModels::OBXD_2Pole,
                {sfpp::PassTypes::Notch, sfpp::DriveTypes::Standard}, 0, 0.5,
                {-11.7372f, -12.9044f, -32.3445f, -12.7962f, -11.6544f});

        runTest({FilterType::fut_obxd_2pole_n,
                 static_cast<FilterSubType>(1),
                 {-11.715f, -12.7591f, -32.0229f, -12.6596f, -11.6536f}});
        runTest(sfpp::FilterModels::OBXD_2Pole, {sfpp::PassTypes::Notch, sfpp::DriveTypes::Pushed},
                0, 0.5, {-11.715f, -12.7591f, -32.0229f, -12.6596f, -11.6536f});
    }

    SECTION("4-pole")
    {
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_6db,
                 {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f}});
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_6db,
                 {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f}});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_12db,
                 {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f}});
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_12db,
                 {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f}});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_18db,
                 {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f}});
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_18db,
                 {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f}});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_24db,
                 {-10.8168, -8.97697, -9.51416, -31.9326, -58.5682}});
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_24db,
                 {-10.8168, -8.97697, -9.51416, -31.9326, -58.5682}});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_broken24db,
                 {-4.7424f, -2.72974f, -2.51195f, -23.2202f, -51.9861f}});
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_broken24db,
                 {-4.7424f, -2.72974f, -2.51195f, -23.2202f, -51.9861f}});
    }
}
