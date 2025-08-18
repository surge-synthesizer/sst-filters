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
        return "Vember Classic";
    case FilterModels::VemberAllPass:
        return "Vember Allpass";
    case FilterModels::VemberLadder:
        return "Vember Ladder";

    case FilterModels::CytomicSVF:
        return "Cytomic SVF";

    case FilterModels::K35:
        return "K35";
    case FilterModels::DiodeLadder:
        return "Diode Ladder";

    case FilterModels::VintageLadder:
        return "Vintage Ladder";

    case FilterModels::CutoffWarp:
        return "Cutoff Warp";
    case FilterModels::ResonanceWarp:
        return "Resonance Warp";

    case FilterModels::OBXD_4Pole:
        return "OB-Xd 24 dB";
    case FilterModels::OBXD_2Pole:
        return "OB-Xd 12 dB";

    case FilterModels::TriPole:
        return "Tri-Pole";
    case FilterModels::Comb:
        return "Comb";
    case FilterModels::SampleAndHold:
        return "Sample & Hold";
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
        return "Lowpass";
    case PassTypes::HP:
        return "Highpass";
    case PassTypes::BP:
        return "Bandpass";
    case PassTypes::Notch:
        return "Notch";
    case PassTypes::Peak:
        return "Peak";
    case PassTypes::AllPass:
        return "Allpass";
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
        return "6 dB";
    case SlopeLevels::Slope_12db:
        return "12 dB";
    case SlopeLevels::Slope_18db:
        return "18 dB";
    case SlopeLevels::Slope_24db:
        return "24 dB";

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
        return "Negative Feedback, Mix 100%";
    case SlopeLevels::Comb_Negative_50:
        return "Negative Feedback, Mix 50%";
    case SlopeLevels::Comb_Positive_50:
        return "Postive Feedback, Mix 50%";
    case SlopeLevels::Comb_Positive_100:
        return "Positive Feedback, Mix 100%";
    case SlopeLevels::Comb_Bipolar_ContinuousMix:
        return "Bipolar, Variable Mix";
    case SlopeLevels::Comb_Positive_ContinuousMix:
        return "Positive Feedback, Variable Mix";
    case SlopeLevels::Comb_Negative_ContinuousMix:
        return "Negative Feedback, Variable Mix";
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
    case DriveTypes::K35_Continuous:
        return "Continuous";

    // For the Cutoff/Resonance Warp
    case DriveTypes::Tanh:
        return "tanh";
    case DriveTypes::SoftClip:
        return "Soft Clip";
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

    // For OB-Xd
    case SubModelTypes::BrokenOBXD4Pole24:
        return "24 dB (Legacy)";

    // For Vintage Ladder
    case SubModelTypes::RungeKutta:
        return "Runge-Kutta";
    case SubModelTypes::RungeKuttaCompensated:
        return "Runge-Kutta Compensated";
    case SubModelTypes::Huov:
        return "Huovilainen";
    case SubModelTypes::HuovCompensated:
        return "Huovilainen Compensated";

    // For Tri-Pole
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
