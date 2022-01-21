#pragma once

#include "catch2/catch2.hpp"

#include <sst/filters.h>

namespace BasicFiltersConstants
{
constexpr auto sampleRate = 48000.0f;
constexpr int blockSize = 128;
} // namespace BasicFiltersConstants

TEST_CASE("Basic Filters")
{
    using sst::filters::FilterType;

    auto runSine = [](auto &filterState, auto &filterUnitPtr, float testFreq, int numSamples) {
        // reset filter state
        std::fill (filterState.R, &filterState.R[sst::filters::n_filter_registers], _mm_setzero_ps());

        std::vector<float> y(numSamples, 0.0f);
        for (int i = 0; i < numSamples; ++i)
        {
            auto x = (float)std::sin(2.0 * M_PI * (double)i * testFreq /
                                     BasicFiltersConstants::sampleRate);

            auto yVec = filterUnitPtr(&filterState, _mm_set_ps1(x));

            float yArr alignas(16)[4];
            _mm_store_ps (yArr, yVec);
            y[i] = yArr[0];
        }

        auto squareSum = std::inner_product(y.begin(), y.end(), y.begin(), 0.0f);
        auto rms = std::sqrt(squareSum / (float) numSamples);
        return 20.0f * std::log10 (rms);
    };

    SECTION("LP_12")
    {
        auto filterState = sst::filters::QuadFilterUnitState{};
        auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(FilterType::fut_lp12, 0);

        sst::filters::FilterCoefficientMaker coefMaker;
        coefMaker.MakeCoeffs(60.0f, 0.5f, FilterType::fut_lp12, 0, nullptr, false);
        coefMaker.castCoefficients(filterState.C, filterState.dC);

        auto rmsDB = runSine (filterState, filterUnitPtr, 100.0f, 512);
        std::cout << rmsDB << std::endl;

        rmsDB = runSine (filterState, filterUnitPtr, 440.0f, 512);
        std::cout << rmsDB << std::endl;

        rmsDB = runSine (filterState, filterUnitPtr, 20000.0f, 512);
        std::cout << rmsDB << std::endl;
    }

    SECTION("LP_24") {}

    SECTION("HP_12") {}
    SECTION("HP_24") {}

    SECTION("BP_12") {}
    SECTION("BP_24") {}

    SECTION("NOTCH_12") {}
    SECTION("NOTCH_24") {}

    SECTION("APF") {}
}
