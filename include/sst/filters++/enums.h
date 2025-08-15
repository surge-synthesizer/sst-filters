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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_ENUMS_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_ENUMS_H

#include <cstdint>
#include <string>

namespace sst::filtersplusplus
{
// If you add a model here, add it to Filter::availableModels in details/filter_impl too
enum struct FilterModels : uint32_t
{
    Off = 0,

    VemberClassic = 0x100,
    VemberAllPass = 0x105,
    VemberLadder = 0x110,

    K35 = 0x300,
    DiodeLadder = 0x310,

    VintageLadder = 0x320,

    CutoffWarp = 0x330,
    ResonanceWarp = 0x340,

    OBXD_4Pole = 0x350,
    OBXD_2Pole = 0x360,

    TriPole = 0x400,
    Comb = 0x410,          // Including pos and neg
    SampleAndHold = 0x420, // Including pos and neg
};

std::string toString(const FilterModels &f);

enum struct PassTypes : uint32_t
{
    UNSUPPORTED = 0,

    LP = 0x04,
    HP = 0x08,
    BP = 0x0C,
    Notch = 0x10,
    Peak = 0x14,
    AllPass = 0x18,
    LowShelf = 0x300,
    Bell = 0x400,
    HighShelf = 0x500
};

std::string toString(const PassTypes &p);

enum struct SlopeLevels : uint32_t
{
    UNSUPPORTED = 0,

    Slope_6db = 0x100,

    Slope_12db = 0x200,

    Slope_18db = 0x300,

    Slope_24db = 0x400,

    Slope_1Stage = 0x450,
    Slope_2Stage = 0x460,
    Slope_3Stage = 0x470,
    Slope_4Stage = 0x480,

    Slope_Morph = 0x500,

    Comb_Negative_ContinuousMix = 0x590,
    Comb_Negative_100 = 0x600,
    Comb_Negative_50 = 0x610,
    Comb_Positive_50 = 0x620,
    Comb_Positive_100 = 0x630,
    Comb_Positive_ContinuousMix = 0x640,
    Comb_Bipolar_ContinuousMix = 0x650
};

std::string toString(const SlopeLevels &s);

enum struct DriveTypes : uint32_t
{
    UNSUPPORTED = 0,

    Standard = 0x10,
    Clean = 0x20,
    Driven = 0x30,
    NotchMild = 0x40,

    K35_None = 0x100,
    K35_Mild = 0x105,
    K35_Moderate = 0x109,
    K35_Heavy = 0x10C,
    K35_Extreme = 0x10F,

    // For the res and cutoff warp circuits
    Tanh = 0x150,
    SoftClip = 0x160,
    OJD = 0x170,

    Pushed = 0x200
};

std::string toString(const DriveTypes &d);

enum struct SubModelTypes : uint32_t
{
    UNSUPPORTED = 0,

    // For obxd
    BrokenOBXD4Pole24 = 0x700,

    // For vintage ladder
    RungeKutta = 0x800,
    RungeKuttaCompensated = 0x801,
    Huov = 0x802,
    HuovCompensated = 0x803,

    // For tripole
    LowLowLow = 0x900,
    LowHighLow = 0x902,
    HighLowHigh = 0x903,
    HighHighHigh = 0x904
};

std::string toString(const SubModelTypes &s);

} // namespace sst::filtersplusplus
#endif // ENUMS_H
