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
inline std::string toString(const FilterModels &f)
{
    switch (f)
    {
    case FilterModels::Off:
        return "Off";
    case FilterModels::VemberClassic:
        return "VemberClassic";
    case FilterModels::VemberAllPass:
        return "VemberAllPass";
    case FilterModels::VemberLadder:
        return "VemberLadder";

    case FilterModels::K35:
        return "K35";
    case FilterModels::DiodeLadder:
        return "DiodeLadder";

    case FilterModels::VintageLadder:
        return "VintageLadder";

    case FilterModels::CutoffWarp:
        return "CutoffWarp";
    case FilterModels::ResonanceWarp:
        return "ResonanceWarp";

    case FilterModels::OBXD_4Pole:
        return "OBXD 4Pole";
    case FilterModels::OBXD_2Pole:
        return "OBXD 2Pole";

    case FilterModels::TriPole:
        return "TriPole";
    case FilterModels::Comb:
        return "Comb";
    case FilterModels::SampleAndHold:
        return "Sample and Hold";
    };
    return "MODEL_ERROR";
}

inline std::string toString(const PassTypes &p)
{
    switch (p)
    {
    case PassTypes::UNSUPPORTED:
        return "UNSUPPORTED";

    case PassTypes::LP:
        return "LowPass";
    case PassTypes::HP:
        return "HighPass";
    case PassTypes::BP:
        return "BandPass";
    case PassTypes::Notch:
        return "Notch";
    case PassTypes::Peak:
        return "Peak";
    case PassTypes::AllPass:
        return "AllPass";
    case PassTypes::LowShelf:
        return "Low Shelf";
    case PassTypes::Bell:
        return "Bell";
    case PassTypes::HighShelf:
        return "High Shelf";
    }
    return "PASSTYPE_ERROR";
}

inline std::string toString(const SlopeLevels &s)
{
    switch (s)
    {
    case SlopeLevels::UNSUPPORTED:
        return "UNSUPPORTED";

    case SlopeLevels::Slope_6db:
        return "6db";
    case SlopeLevels::Slope_12db:
        return "12db";
    case SlopeLevels::Slope_18db:
        return "18db";
    case SlopeLevels::Slope_24db:
        return "24db";

    case SlopeLevels::Slope_1Stage:
        return "1 Stage";
    case SlopeLevels::Slope_2Stage:
        return "2 Stage";
    case SlopeLevels::Slope_3Stage:
        return "3 Stage";
    case SlopeLevels::Slope_4Stage:
        return "4 Stage";

    case SlopeLevels::Slope_Morph:
        return "Morph";

    case SlopeLevels::Comb_Negative_100:
        return "Negative FB, Mix 100%";
    case SlopeLevels::Comb_Negative_50:
        return "Negative FB, Mix 50%";
    case SlopeLevels::Comb_Positive_50:
        return "Postive FB, Mix 50%";
    case SlopeLevels::Comb_Positive_100:
        return "Positive FB, Mix 100%";
    case SlopeLevels::Comb_Bipolar_ContinuousMix:
        return "Bipolar Continuous Mix";
    case SlopeLevels::Comb_Positive_ContinuousMix:
        return "Positive FB, Mix Variable";
    case SlopeLevels::Comb_Negative_ContinuousMix:
        return "Negative FB, Mix Variable";
    }
    return "SLOPELEVELS_ERROR";
}

inline std::string toString(const DriveTypes &d)
{
    switch (d)
    {
    case DriveTypes::UNSUPPORTED:
        return "UNSUPPORTED";

    case DriveTypes::Standard:
        return "Standard";
    case DriveTypes::Clean:
        return "Clean";
    case DriveTypes::Driven:
        return "Driven";
    case DriveTypes::NotchMild:
        return "Mild";

    case DriveTypes::K35_None:
        return "None";
    case DriveTypes::K35_Mild:
        return "Mild";
    case DriveTypes::K35_Moderate:
        return "Moderate";
    case DriveTypes::K35_Heavy:
        return "Heavy";
    case DriveTypes::K35_Extreme:
        return "Extreme";

    // For the res and cutoff warp circuits
    case DriveTypes::Tanh:
        return "Tanh";
    case DriveTypes::SoftClip:
        return "SoftClip";
    case DriveTypes::OJD:
        return "OJD";

    case DriveTypes::Pushed:
        return "Pushed";
    }
    return "DRIVETYPES_ERROR";
}

inline std::string toString(const SubModelTypes &s)
{
    switch (s)
    {
    case SubModelTypes::UNSUPPORTED:
        return "UNSUPPORTED";

    // For obxd
    case SubModelTypes::BrokenOBXD4Pole24:
        return "Broken Legacy 24db";

    // For vintage ladder
    case SubModelTypes::RungeKutta:
        return "RungeKutta";
    case SubModelTypes::RungeKuttaCompensated:
        return "RungeKutta Compensated";
    case SubModelTypes::Huov:
        return "Huov";
    case SubModelTypes::HuovCompensated:
        return "Huov Compensated";

    // For tripole
    case SubModelTypes::LowLowLow:
        return "Low/Low/Low";
    case SubModelTypes::LowHighLow:
        return "Low/High/Low";
    case SubModelTypes::HighLowHigh:
        return "High/Low/High";
    case SubModelTypes::HighHighHigh:
        return "High/High/High";
    }
    return "SUBMODEL_ERROR";
}

} // namespace sst::filtersplusplus
#endif // ENUMS_TO_STRING_H
