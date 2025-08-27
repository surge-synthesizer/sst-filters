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

TEST_CASE("OB-Xd Filter")
{
    using namespace TestUtils;
    namespace sfpp = sst::filtersplusplus;

    SECTION("2-pole Lowpass")
    {
        runTest({FilterType::fut_obxd_2pole_lp,
                 static_cast<FilterSubType>(0),
                 {-5.42604f, -4.8885f, -5.72345f, -18.9727f, -54.1739f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::LP, sfpp::DriveMode::Standard}, 0,
                0.5, {-5.42604f, -4.8885f, -5.72345f, -18.9727f, -54.1739f});

        runTest({FilterType::fut_obxd_2pole_lp,
                 static_cast<FilterSubType>(1),
                 {-5.40362f, -4.74114f, -5.10849f, -18.8247f, -53.902f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::LP, sfpp::DriveMode::Pushed}, 0,
                0.5, {-5.40362f, -4.74114f, -5.10849f, -18.8247f, -53.902f});
    }

    SECTION("2-pole Bandpass")
    {
        runTest({FilterType::fut_obxd_2pole_bp,
                 static_cast<FilterSubType>(0),
                 {-26.4609f, -17.8167f, -11.7726f, -18.0352f, -40.1138f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::BP, sfpp::DriveMode::Standard}, 0,
                0.5, {-26.4609f, -17.8167f, -11.7726f, -18.0352f, -40.1138f});

        runTest({FilterType::fut_obxd_2pole_bp,
                 static_cast<FilterSubType>(1),
                 {-26.4323f, -17.6603f, -11.1551f, -17.8948f, -40.1103f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::BP, sfpp::DriveMode::Pushed}, 0,
                0.5, {-26.4323f, -17.6603f, -11.1551f, -17.8948f, -40.1103f});
    }

    SECTION("2-pole Highpass")
    {
        runTest({FilterType::fut_obxd_2pole_hp,
                 static_cast<FilterSubType>(0),
                 {-34.0961f, -18.4561f, -5.7608f, -4.90633f, -5.62153f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::HP, sfpp::DriveMode::Standard}, 0,
                0.5, {-34.0961f, -18.4561f, -5.7608f, -4.90633f, -5.62153f});

        runTest({FilterType::fut_obxd_2pole_hp,
                 static_cast<FilterSubType>(1),
                 {-34.0065f, -18.2937f, -5.14328f, -4.76908f, -5.62067f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::HP, sfpp::DriveMode::Pushed}, 0,
                0.5, {-34.0065f, -18.2937f, -5.14328f, -4.76908f, -5.62067f});
    }

    SECTION("2-pole Notch")
    {
        runTest({FilterType::fut_obxd_2pole_n,
                 static_cast<FilterSubType>(0),
                 {-11.7372f, -12.9044f, -32.3445f, -12.7962f, -11.6544f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::Notch, sfpp::DriveMode::Standard},
                0, 0.5, {-11.7372f, -12.9044f, -32.3445f, -12.7962f, -11.6544f});

        runTest({FilterType::fut_obxd_2pole_n,
                 static_cast<FilterSubType>(1),
                 {-11.715f, -12.7591f, -32.0229f, -12.6596f, -11.6536f}});
        runTest(sfpp::FilterModel::OBXD_2Pole, {sfpp::Passband::Notch, sfpp::DriveMode::Pushed}, 0,
                0.5, {-11.715f, -12.7591f, -32.0229f, -12.6596f, -11.6536f});
    }

    SECTION("4-pole")
    {
        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_6dB,
                 {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f}});
        runTest(sfpp::FilterModel::OBXD_4Pole, {sfpp::Passband::LP, sfpp::Slope::Slope_6dB}, 0, 0.5,
                {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_12dB,
                 {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f}});
        runTest(sfpp::FilterModel::OBXD_4Pole, {sfpp::Passband::LP, sfpp::Slope::Slope_12dB}, 0,
                0.5, {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_18dB,
                 {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f}});
        runTest(sfpp::FilterModel::OBXD_4Pole, {sfpp::Passband::LP, sfpp::Slope::Slope_18dB}, 0,
                0.5, {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_24dB,
                 {-10.8168, -8.97697, -9.51416, -31.9326, -58.5682}});
        runTest(sfpp::FilterModel::OBXD_4Pole, {sfpp::Passband::LP, sfpp::Slope::Slope_24dB}, 0,
                0.5, {-10.8168, -8.97697, -9.51416, -31.9326, -58.5682});

        runTest({FilterType::fut_obxd_4pole,
                 FilterSubType::st_obxd4pole_broken24dB,
                 {-4.7424f, -2.72974f, -2.51195f, -23.2202f, -51.9861f}});
        runTest(
            sfpp::FilterModel::OBXD_4Pole,
            {sfpp::Passband::LP, sfpp::Slope::Slope_24dB, sfpp::FilterSubModel::BrokenOBXD4Pole24},
            0, 0.5, {-4.7424f, -2.72974f, -2.51195f, -23.2202f, -51.9861f});
    }

    SECTION("Xpander")
    {
        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_lp1,
                 {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::LP, sfpp::Slope::Slope_6dB}, 0,
                0.5, {-10.3241f, -6.48075f, -0.536573f, -10.2352f, -30.8738f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_lp2,
                 {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::LP, sfpp::Slope::Slope_12dB}, 0,
                0.5, {-10.4795f, -7.32661f, -3.53102f, -18.0087f, -53.1575f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_lp3,
                 {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::LP, sfpp::Slope::Slope_18dB}, 0,
                0.5, {-10.6428f, -8.15069f, -6.50921f, -25.4671f, -56.6591f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_lp4,
                 {-10.8168, -8.97697, -9.51416, -31.9326, -58.5682}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::LP, sfpp::Slope::Slope_24dB}, 0,
                0.5, {-10.8168, -8.97697, -9.51416, -31.9326, -58.5682});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_hp1,
                 {-24.9438f, -13.1056f, -0.528926f, -3.15228f, -2.41054f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::HP, sfpp::Slope::Slope_6dB}, 0,
                0.5, {-24.9438f, -13.1056f, -0.528926f, -3.15228f, -2.41054f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_hp2,
                 {-37.2119f, -20.5849f, -3.61575f, -3.92377f, -2.41679f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::HP, sfpp::Slope::Slope_12dB}, 0,
                0.5, {-37.2119f, -20.5849f, -3.61575f, -3.92377f, -2.41679f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_hp3,
                 {-43.75f, -27.6081f, -6.68905f, -4.69537f, -2.42307f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::HP, sfpp::Slope::Slope_18dB}, 0,
                0.5, {-43.75f, -27.6081f, -6.68905f, -4.69537f, -2.42307f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_bp2,
                 {-19.2489f, -8.01136f, 2.45672f, -5.02172f, -24.879f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::BP, sfpp::Slope::Slope_12dB}, 0,
                0.5, {-19.2489f, -8.01136f, 2.45672f, -5.02172f, -24.879f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_bp4,
                 {-33.002f, -16.4243f, -3.60726f, -13.6689f, -51.1749f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::BP, sfpp::Slope::Slope_24dB}, 0,
                0.5, {-33.002f, -16.4243f, -3.60726f, -13.6689f, -51.1749f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_n2,
                 {-10.7763f, -9.36031f, -25.7763f, -5.79322f, -2.4291f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::Notch, sfpp::Slope::Slope_12dB},
                0, 0.5, {-10.7763f, -9.36031f, -25.7763f, -5.79322f, -2.4291f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_ph3,
                 {-16.1412f, -6.06469f, -0.523086f, -12.1305f, -2.4599f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::Phaser, sfpp::Slope::Slope_18dB},
                0, 0.5, {-16.1412f, -6.06469f, -0.523086f, -12.1305f, -2.4599f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_hp2lp1,
                 {-38.2663f, -21.5233f, -6.59278f, -11.8341f, -30.9139f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::BP, sfpp::Slope::Slope_12dB6dB},
                0, 0.5, {-38.2663f, -21.5233f, -6.59278f, -11.8341f, -30.9139f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_hp3lp1,
                 {-46.3801f, -28.7917f, -9.65358f, -12.6151f, -30.9242f}});
        runTest(sfpp::FilterModel::OBXD_Xpander, {sfpp::Passband::BP, sfpp::Slope::Slope_18dB6dB},
                0, 0.5, {-46.3801f, -28.7917f, -9.65358f, -12.6151f, -30.9242f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_n2lp1,
                 {-10.939f, -10.18f, -28.1791f, -13.7106f, -30.9311f}});
        runTest(sfpp::FilterModel::OBXD_Xpander,
                {sfpp::Passband::NotchAndLP, sfpp::Slope::Slope_12dB6dB}, 0, 0.5,
                {-10.939f, -10.18f, -28.1791f, -13.7106f, -30.9311f});

        runTest({FilterType::fut_obxd_xpander,
                 FilterSubType::st_obxdxpander_ph3lp1,
                 {-16.3624f, -6.96748f, -3.53552f, -19.8704f, -30.9486f}});
        runTest(sfpp::FilterModel::OBXD_Xpander,
                {sfpp::Passband::PhaserAndLP, sfpp::Slope::Slope_18dB6dB}, 0, 0.5,
                {-16.3624f, -6.96748f, -3.53552f, -19.8704f, -30.9486f});
    }
}
