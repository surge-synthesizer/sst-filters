#pragma once

#include "catch2/catch2.hpp"

#include <sst/filters.h>

TEST_CASE("Basic Filters")
{
    struct DummyTuningProvider
    {
        enum Mode
        {
            RETUNE_ALL,
            RETUNE_NONE,
        };

        Mode tuningApplicationMode = RETUNE_NONE;

        float note_to_pitch(float note) { return pow(2.0f, note / 12.f); }
    };

    using sst::filters::FilterType;

    SECTION("LP_12")
    {
        auto filterState = sst::filters::QuadFilterUnitState{};
        auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(FilterType::fut_lp12, 0);

        sst::filters::FilterCoefficientMaker<DummyTuningProvider> coefMaker;
        coefMaker.MakeCoeffs(100.0f, 0.7071f, FilterType::fut_lp12, 0, nullptr, false);
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
