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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_ENUMS_TO_STRING_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_ENUMS_TO_STRING_H

#include <string>
#include "enums.h"

namespace sst::filtersplusplus
{
inline std::string toString(const FilterModel &f)
{
    switch (f)
    {
    case FilterModel::None:
        return "None";
    case FilterModel::VemberClassic:
        return "Vember Classic";
    case FilterModel::VemberLadder:
        return "Vember Ladder";

    case FilterModel::CytomicSVF:
        return "Fast SVF";

    case FilterModel::K35:
        return "K35";
    case FilterModel::DiodeLadder:
        return "Diode Ladder";

    case FilterModel::VintageLadder:
        return "Vintage Ladder";

    case FilterModel::CutoffWarp:
        return "Cutoff Warp";
    case FilterModel::ResonanceWarp:
        return "Resonance Warp";

    case FilterModel::OBXD_2Pole:
        return "OB-Xd 2-pole";
    case FilterModel::OBXD_4Pole:
        return "OB-Xd 4-pole";
    case FilterModel::OBXD_Xpander:
        return "OB-Xd Xpander";

    case FilterModel::TriPole:
        return "Tri-pole";
    case FilterModel::Comb:
        return "Comb";
    case FilterModel::SampleAndHold:
        return "Sample & Hold";
    };
    return "MODEL_ERROR";
}

inline std::string toString(const Passband &p)
{
    switch (p)
    {
    case Passband::UNSUPPORTED:
        return "UNSUPPORTED";

    case Passband::LP:
        return "Lowpass";
    case Passband::HP:
        return "Highpass";
    case Passband::BP:
        return "Bandpass";
    case Passband::Notch:
        return "Notch";
    case Passband::Peak:
        return "Peak";
    case Passband::Allpass:
        return "Allpass";
    case Passband::LowShelf:
        return "Low Shelf";
    case Passband::Bell:
        return "Bell";
    case Passband::HighShelf:
        return "High Shelf";
    case Passband::Phaser:
        return "Phaser";
    case Passband::NotchAndLP:
        return "Notch+LP";
    case Passband::PhaserAndLP:
        return "Phaser+LP";

        // For Tri-Pole
    case Passband::LowLowLow:
        return "Low/Low/Low";
    case Passband::LowHighLow:
        return "Low/High/Low";
    case Passband::HighLowHigh:
        return "High/Low/High";
    case Passband::HighHighHigh:
        return "High/High/High";
    }
    return "PASSTYPE_ERROR";
}

inline std::string toString(const Slope &s)
{
    switch (s)
    {
    case Slope::UNSUPPORTED:
        return "UNSUPPORTED";

    case Slope::Slope_6dB:
        return "6 dB";
    case Slope::Slope_12dB:
        return "12 dB";
    case Slope::Slope_18dB:
        return "18 dB";
    case Slope::Slope_24dB:
        return "24 dB";

    case Slope::Slope_6dB12dB:
        return "6 dB/12 dB";
    case Slope::Slope_6dB18dB:
        return "6 dB/18 dB";
    case Slope::Slope_12dB6dB:
        return "12 dB/6 dB";
    case Slope::Slope_18dB6dB:
        return "18 dB/6 dB";

    case Slope::Slope_Morph:
        return "Morph";

    case Slope::Comb_Negative_100:
        return "Negative Feedback, Mix 100%";
    case Slope::Comb_Negative_50:
        return "Negative Feedback, Mix 50%";
    case Slope::Comb_Positive_50:
        return "Postive Feedback, Mix 50%";
    case Slope::Comb_Positive_100:
        return "Positive Feedback, Mix 100%";
    case Slope::Comb_Bipolar_ContinuousMix:
        return "Bipolar, Variable Mix";
    case Slope::Comb_Positive_ContinuousMix:
        return "Positive Feedback, Variable Mix";
    case Slope::Comb_Negative_ContinuousMix:
        return "Negative Feedback, Variable Mix";
    }
    return "SLOPELEVELS_ERROR";
}

inline std::string toString(const DriveMode &d)
{
    switch (d)
    {
    case DriveMode::UNSUPPORTED:
        return "UNSUPPORTED";

    case DriveMode::Standard:
        return "Standard";
    case DriveMode::Clean:
        return "Clean";
    case DriveMode::Driven:
        return "Driven";
    case DriveMode::NotchMild:
        return "Mild";

    case DriveMode::K35_None:
        return "None";
    case DriveMode::K35_Mild:
        return "Mild";
    case DriveMode::K35_Moderate:
        return "Moderate";
    case DriveMode::K35_Heavy:
        return "Heavy";
    case DriveMode::K35_Extreme:
        return "Extreme";
    case DriveMode::K35_Continuous:
        return "Continuous";

        // For Cutoff/Resonance Warp
    case DriveMode::Tanh:
        return "tanh";
    case DriveMode::SoftClip:
        return "Soft Clip";
    case DriveMode::OJD:
        return "OJD";

    case DriveMode::Pushed:
        return "Pushed";
    }
    return "DRIVETYPES_ERROR";
}

inline std::string toString(const FilterSubModel &s)
{
    switch (s)
    {
    case FilterSubModel::UNSUPPORTED:
        return "UNSUPPORTED";

    // For OB-Xd
    case FilterSubModel::BrokenOBXD4Pole24:
        return "24 dB (Legacy)";

    // For Vintage Ladder
    case FilterSubModel::RungeKutta:
        return "Runge-Kutta";
    case FilterSubModel::RungeKuttaCompensated:
        return "Runge-Kutta Compensated";
    case FilterSubModel::Huov:
        return "Huovilainen";
    case FilterSubModel::HuovCompensated:
        return "Huovilainen Compensated";

    // For Tri-Pole
    case FilterSubModel::First_output:
        return "First";
    case FilterSubModel::Second_output:
        return "Second";
    case FilterSubModel::Third_output:
        return "Third";

        // For Cutoff/Resonance Warp
    case FilterSubModel::Warp_1Stage:
        return "1 Stage";
    case FilterSubModel::Warp_2Stage:
        return "2 Stage";
    case FilterSubModel::Warp_3Stage:
        return "3 Stage";
    case FilterSubModel::Warp_4Stage:
        return "4 Stage";
    }
    return "SUBMODEL_ERROR";
}

} // namespace sst::filtersplusplus
#endif // ENUMS_TO_STRING_H
