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

TEST_CASE("Vintage Ladders")
{
    using namespace TestUtils;
    namespace sfpp = sst::filtersplusplus;

    SECTION("Runge-Kutta")
    {
        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(0),
                 {-17.9486f, -16.0861f, -13.4772f, -37.4639f, -64.0226f}});

        runTest(sfpp::FilterModel::VintageLadder,
                {sfpp::Passband::LP, sfpp::FilterSubModel::RungeKutta}, 0, 0.5,
                {-17.9486f, -16.0861f, -13.4772f, -37.4639f, -64.0226f});

        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(1),
                 {-10.7992f, -8.91631f, -8.36544f, -30.9535f, -57.8073f}});

        runTest(sfpp::FilterModel::VintageLadder,
                {sfpp::Passband::LP, sfpp::FilterSubModel::RungeKuttaCompensated}, 0, 0.5,
                {-10.7992f, -8.91631f, -8.36544f, -30.9535f, -57.8073f});
    }

    SECTION("Huovilainen")
    {
        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(2),
                 {-12.251f, -10.3796f, -9.22999f, -32.5839f, -59.1211f}});

        runTest(sfpp::FilterModel::VintageLadder, {sfpp::Passband::LP, sfpp::FilterSubModel::Huov},
                0, 0.5, {-12.251f, -10.3796f, -9.22999f, -32.5839f, -59.1211f});

        runTest({FilterType::fut_vintageladder,
                 static_cast<FilterSubType>(3),
                 {-6.17262f, -4.30116f, -3.1663f, -26.5073f, -53.0447f}});

        runTest(sfpp::FilterModel::VintageLadder,
                {sfpp::Passband::LP, sfpp::FilterSubModel::HuovCompensated}, 0, 0.5,
                {-6.17262f, -4.30116f, -3.1663f, -26.5073f, -53.0447f});
    }
}
