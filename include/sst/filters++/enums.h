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
// I'm also generally trying to keep the enum values below 0xFF but nothing will break if you don't
enum struct FilterModel : uint32_t
{
    None = 0,

    VemberClassic = 0x10,
    VemberLadder = 0x18,

    CytomicSVF = 0x20,

    K35 = 0x30,
    DiodeLadder = 0x35,

    OBXD_4Pole = 0x40,
    OBXD_2Pole = 0x45,
    OBXD_Xpander = 0x47,

    VintageLadder = 0x50,

    CutoffWarp = 0x60,
    ResonanceWarp = 0x65,

    TriPole = 0x70,
    Comb = 0x80, // Including positive and negative
    SampleAndHold = 0x85
};

std::string toString(const FilterModel &f);

enum struct Passband : uint32_t
{
    UNSUPPORTED = 0,

    LP = 0x04,
    HP = 0x08,
    BP = 0x0C,
    Notch = 0x10,
    Peak = 0x14,
    Allpass = 0x18,

    LowShelf = 0x50,
    Bell = 0x60,
    HighShelf = 0x70,

    Phaser = 0x80,

    NotchAndLP = 0x92,
    PhaserAndLP = 0x94,

    // For Tri-pole
    LowLowLow = 0x30 + 0b000,
    LowHighLow = 0x30 + 0b010,
    HighLowHigh = 0x30 + 0b101,
    HighHighHigh = 0x30 + 0b111
};

std::string toString(const Passband &p);

enum struct Slope : uint32_t
{
    UNSUPPORTED = 0,

    Slope_6dB = 0x06,
    Slope_12dB = 0x12,
    Slope_18dB = 0x18,
    Slope_24dB = 0x24,

    // pole-mixing allows for various asymmetric bandpass and combined passband responses...
    Slope_6dB12dB = 0x07,
    Slope_6dB18dB = 0x08,
    Slope_12dB6dB = 0x13,
    Slope_18dB6dB = 0x19,

    Slope_Morph = 0x40,

    Comb_Negative_ContinuousMix = 0x50,
    Comb_Negative_100 = 0x51,
    Comb_Negative_50 = 0x52,
    Comb_Positive_50 = 0x53,
    Comb_Positive_100 = 0x54,
    Comb_Positive_ContinuousMix = 0x55,
    Comb_Bipolar_ContinuousMix = 0x56
};

std::string toString(const Slope &s);

enum struct DriveMode : uint32_t
{
    UNSUPPORTED = 0,

    Standard = 0x10,
    Clean = 0x12,
    Driven = 0x14,
    NotchMild = 0x18,

    K35_None = 0x30,
    K35_Mild = 0x32,
    K35_Moderate = 0x34,
    K35_Heavy = 0x36,
    K35_Extreme = 0x38,
    K35_Continuous = 0x39,

    Pushed = 0x50,

    // For the res and cutoff warp circuits
    Tanh = 0x40,
    SoftClip = 0x42,
    OJD = 0x44,

};

std::string toString(const DriveMode &d);

enum struct FilterSubModel : uint32_t
{
    UNSUPPORTED = 0,

    // For OB-Xd
    BrokenOBXD4Pole24 = 0x10,

    // For Vintage Ladder
    RungeKutta = 0x20,
    RungeKuttaCompensated = 0x22,
    Huov = 0x24,
    HuovCompensated = 0x26,

    // For Tripole
    First_output = 0x31,
    Second_output = 0x32,
    Third_output = 0x33,

    // For the res and cutoff warp circuits
    Warp_1Stage = 0x60,
    Warp_2Stage = 0x61,
    Warp_3Stage = 0x62,
    Warp_4Stage = 0x63,
};

std::string toString(const FilterSubModel &s);

} // namespace sst::filtersplusplus
#endif // ENUMS_H
