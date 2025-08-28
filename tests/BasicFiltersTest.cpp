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

TEST_CASE("Basic Filters")
{
    using namespace TestUtils;
    namespace sfpp = sst::filtersplusplus;

    SECTION("LP 12")
    {
        runTest({FilterType::fut_lp12,
                 FilterSubType::st_Standard,
                 {-8.02604f, -6.72912f, -3.8718f, -20.6177f, -53.7828f}});

        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::LP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Standard}, 0.f, 0.5,
                {-8.02604f, -6.72912f, -3.8718f, -20.6177f, -53.7828f});

        runTest({FilterType::fut_lp12,
                 FilterSubType::st_Driven,
                 {-4.52473f, -3.6442f, -2.13439f, -16.4384f, -49.3718f},
                 0.0f});

        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::LP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Driven}, 0.f, 0.5,
                {-4.52473f, -3.6442f, -2.13439f, -16.4384f, -49.3718f});

        runTest({FilterType::fut_lp12,
                 FilterSubType::st_Clean,
                 {-3.62033f, -3.70258f, -6.10753f, -17.6442f, -53.1438f}});

        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::LP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Clean}, 0.f, 0.5,
                {-3.62033f, -3.70258f, -6.10753f, -17.6442f, -53.1438f});
    }

    SECTION("LP 24")
    {
        runTest({FilterType::fut_lp24,
                 FilterSubType::st_Standard,
                 {-7.79654f, -5.2026f, -1.93057f, -27.0258f, -50.9426f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::LP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Standard}, 0.f, 0.5,
                {-7.79654f, -5.2026f, -1.93057f, -27.0258f, -50.9426f});

        runTest({FilterType::fut_lp24,
                 FilterSubType::st_Driven,
                 {-6.51442f, -5.74044f, -6.61548f, -28.9689f, -54.0992f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::LP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Driven}, 0.f, 0.5,
                {-6.51442f, -5.74044f, -6.61548f, -28.9689f, -54.0992f});

        runTest({FilterType::fut_lp24,
                 FilterSubType::st_Clean,
                 {-5.89545f, -10.7429f, -20.97f, -35.7356f, -60.1235f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::LP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Clean}, 0.f, 0.5,
                {-5.89545f, -10.7429f, -20.97f, -35.7356f, -60.1235f});
    }

    SECTION("HP 12")
    {
        runTest({FilterType::fut_hp12,
                 FilterSubType::st_Standard,
                 {-35.8899f, -20.1651f, -3.91549f, -6.78058f, -8.34447f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::HP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Standard}, 0.f, 0.5,
                {-35.8899f, -20.1651f, -3.91549f, -6.78058f, -8.34447f});

        runTest({FilterType::fut_hp12,
                 FilterSubType::st_Driven,
                 {-31.4426f, -15.8495f, -1.84373f, -2.67782f, -4.16204f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::HP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Driven}, 0.f, 0.5,
                {-31.4426f, -15.8495f, -1.84373f, -2.67782f, -4.16204f});

        runTest({FilterType::fut_hp12,
                 FilterSubType::st_Clean,
                 {-32.3916f, -17.1967f, -6.15529f, -3.55761f, -3.57168f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::HP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Clean}, 0.f, 0.5,
                {-32.3916f, -17.1967f, -6.15529f, -3.55761f, -3.57168f});
    }

    SECTION("HP 24")
    {
        runTest({FilterType::fut_hp24,
                 FilterSubType::st_Standard,
                 {-38.0661f, -27.0505f, -1.89886f, -5.27993f, -8.33136f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::HP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Standard}, 0.f, 0.5,
                {-38.0661f, -27.0505f, -1.89886f, -5.27993f, -8.33136f});

        runTest({FilterType::fut_hp24,
                 FilterSubType::st_Driven,
                 {-42.8669f, -28.9289f, -6.65685f, -4.0466f, -5.3227f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::HP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Driven}, 0.f, 0.5,
                {-42.8669f, -28.9289f, -6.65685f, -4.0466f, -5.3227f});

        runTest({FilterType::fut_hp24,
                 FilterSubType::st_Clean,
                 {-51.1071f, -34.1528f, -16.2113f, -7.22786f, -4.15892f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::HP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Clean}, 0.f, 0.5,
                {-51.1071f, -34.1528f, -16.2113f, -7.22786f, -4.15892f});
    }

    SECTION("BP 12")
    {
        runTest({FilterType::fut_bp12,
                 FilterSubType::st_Standard,
                 {-22.9694f, -13.5409f, -3.81424f, -13.7711f, -34.7354f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::BP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Standard}, 0, 0.5,
                {-22.9694f, -13.5409f, -3.81424f, -13.7711f, -34.7354f});

        runTest({FilterType::fut_bp12,
                 FilterSubType::st_Driven,
                 {-12.6518f, -3.51403f, 0.968043f, -3.7741f, -26.5632f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::BP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Driven}, 0, 0.5,
                {-12.6518f, -3.51403f, 0.968043f, -3.7741f, -26.5632f});

        runTest({FilterType::fut_bp12,
                 FilterSubType::st_Clean,
                 {-18.5306f, -10.5263f, -6.14993f, -10.6699f, -32.0521f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::BP, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Clean}, 0, 0.5,
                {-18.5306f, -10.5263f, -6.14993f, -10.6699f, -32.0521f});

        runTest({FilterType::fut_bp12,
                 FilterSubType::st_bp12_LegacyDriven,
                 {-12.6309f, -3.79841f, -0.00540512f, -4.4129f, -26.6096f}});

        runTest({FilterType::fut_bp12,
                 FilterSubType::st_bp12_LegacyClean,
                 {-18.7297f, -11.5658f, -9.04975f, -13.323f, -34.3029f}});
    }

    SECTION("BP 24")
    {
        runTest({FilterType::fut_bp24,
                 FilterSubType::st_Standard,
                 {-33.6982f, -18.0861f, -1.37863f, -18.7074f, -50.5816f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::BP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Standard}, 0, 0.5,
                {-33.6982f, -18.0861f, -1.37863f, -18.7074f, -50.5816f});

        runTest({FilterType::fut_bp24,
                 FilterSubType::st_Driven,
                 {-21.5631f, -6.61244f, -0.148954f, -6.07533f, -43.7467f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::BP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Driven}, 0, 0.5,
                {-21.5631f, -6.61244f, -0.148954f, -6.07533f, -43.7467f});

        runTest({FilterType::fut_bp24,
                 FilterSubType::st_Clean,
                 {-39.4046f, -26.4753f, -21.4875f, -26.704f, -65.0009f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::BP, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Clean}, 0, 0.5,
                {-39.4046f, -26.4753f, -21.4875f, -26.704f, -65.0009f});
    }

    SECTION("NOTCH 12")
    {
        runTest({FilterType::fut_notch12,
                 FilterSubType::st_Notch,
                 {-3.76908f, -5.30151f, -24.7274f, -4.76339f, -3.02366f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::Notch, sfpp::Slope::Slope_12dB, sfpp::DriveMode::Standard}, 0, 0.5,
                {-3.76908f, -5.30151f, -24.7274f, -4.76339f, -3.02366f});

        runTest({FilterType::fut_notch12,
                 FilterSubType::st_NotchMild,
                 {-3.2567f, -3.7687f, -20.727f, -3.3948f, -3.01445f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::Notch, sfpp::Slope::Slope_12dB, sfpp::DriveMode::NotchMild}, 0,
                0.5, {-3.2567f, -3.7687f, -20.727f, -3.3948f, -3.01445f});
    }

    SECTION("NOTCH 24")
    {
        runTest({FilterType::fut_notch24,
                 FilterSubType::st_Notch,
                 {-4.4493f, -7.18585f, -27.8142f, -6.48846f, -3.03517f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::Notch, sfpp::Slope::Slope_24dB, sfpp::DriveMode::Standard}, 0, 0.5,
                {-4.4493f, -7.18585f, -27.8142f, -6.48846f, -3.03517f});

        runTest({FilterType::fut_notch24,
                 FilterSubType::st_NotchMild,
                 {-3.55329f, -4.38844f, -23.8628f, -3.76166f, -3.0168f}});
        runTest(sfpp::FilterModel::VemberClassic,
                {sfpp::Passband::Notch, sfpp::Slope::Slope_24dB, sfpp::DriveMode::NotchMild}, 0,
                0.5, {-3.55329f, -4.38844f, -23.8628f, -3.76166f, -3.0168f});
    }

    SECTION("ALLPASS")
    {
        runTest({FilterType::fut_apf,
                 FilterSubType{},
                 {-4.0896f, -4.12088f, -4.06745f, -3.19854f, -3.01547f}});
        runTest(sfpp::FilterModel::VemberClassic, {sfpp::Passband::Allpass}, 0, 0.5,
                {-4.0896f, -4.12088f, -4.06745f, -3.19854f, -3.01547f});
    }

    SECTION("LP MOOG")
    {
        runTest({FilterType::fut_lpmoog,
                 FilterSubType::st_lpmoog_6dB,
                 {-8.26648f, -4.43419f, 4.17682f, -6.93047f, -25.634f}});
        runTest(sfpp::FilterModel::VemberLadder, {sfpp::Passband::LP, sfpp::Slope::Slope_6dB}, 0,
                0.5, {-8.26648f, -4.43419f, 4.17682f, -6.93047f, -25.634f});

        runTest({FilterType::fut_lpmoog,
                 FilterSubType::st_lpmoog_12dB,
                 {-8.42218f, -5.29444f, 1.17737f, -14.6702f, -48.285f}});
        runTest(sfpp::FilterModel::VemberLadder, {sfpp::Passband::LP, sfpp::Slope::Slope_12dB}, 0,
                0.5, {-8.42218f, -5.29444f, 1.17737f, -14.6702f, -48.285f});

        runTest({FilterType::fut_lpmoog,
                 FilterSubType::st_lpmoog_18dB,
                 {-8.58346f, -6.12644f, -1.80083f, -22.0866f, -53.0664f}});
        runTest(sfpp::FilterModel::VemberLadder, {sfpp::Passband::LP, sfpp::Slope::Slope_18dB}, 0,
                0.5, {-8.58346f, -6.12644f, -1.80083f, -22.0866f, -53.0664f});

        runTest({FilterType::fut_lpmoog,
                 FilterSubType::st_lpmoog_24dB,
                 {-8.75385f, -6.95123f, -4.80331f, -28.5335f, -55.2077f}});
        runTest(sfpp::FilterModel::VemberLadder, {sfpp::Passband::LP, sfpp::Slope::Slope_24dB}, 0,
                0.5, {-8.75385f, -6.95123f, -4.80331f, -28.5335f, -55.2077f});
    }

    SECTION("S&H")
    {
        runTest({FilterType::fut_SNH,
                 FilterSubType{},
                 {-5.62205f, -1.85607f, -34.0156f, -4.31724f, -4.09113f}});
        runTest(sfpp::FilterModel::SampleAndHold, {}, 0.0, 0.5,
                {-5.62205f, -1.85607f, -34.0156f, -4.31724f, -4.09113f});
    }

    SECTION("COMB")
    {
        runTest({FilterType::fut_comb_pos,
                 static_cast<FilterSubType>(0),
                 {-6.08834f, -12.7629f, -2.50065f, -9.49002f, -9.36519f}});
        runTest(sfpp::FilterModel::Comb, {sfpp::Slope::Comb_Positive_50}, 0, 0.5,
                {-6.08834f, -12.7629f, -2.50065f, -9.49002f, -9.36519f});

        runTest({FilterType::fut_comb_pos,
                 static_cast<FilterSubType>(1),
                 {-3.68299f, -5.76509f, -1.83625f, -4.67708f, -8.57929f}});
        runTest(sfpp::FilterModel::Comb, {sfpp::Slope::Comb_Positive_100}, 0, 0.5,
                {-3.68299f, -5.76509f, -1.83625f, -4.67708f, -8.57929f});

        runTest({FilterType::fut_comb_neg,
                 static_cast<FilterSubType>(0),
                 {-4.93593f, -12.857f, -4.65075f, -5.64365f, -7.50512f}});
        runTest(sfpp::FilterModel::Comb, {sfpp::Slope::Comb_Negative_50}, 0, 0.5,
                {-4.93593f, -12.857f, -4.65075f, -5.64365f, -7.50512f});

        runTest({FilterType::fut_comb_neg,
                 static_cast<FilterSubType>(1),
                 {-5.81567f, -2.17117f, -6.19879f, -4.28433f, -8.0815f}});
        runTest(sfpp::FilterModel::Comb, {sfpp::Slope::Comb_Negative_100}, 0, 0.5,
                {-5.81567f, -2.17117f, -6.19879f, -4.28433f, -8.0815f});
    }
}
