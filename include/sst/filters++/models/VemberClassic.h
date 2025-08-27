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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_VEMBERCLASSIC_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_VEMBERCLASSIC_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::vemberclassic
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Passband::LP, Slope::Slope_12dB, DriveMode::Standard},
         {sft::FilterType::fut_lp12, sft::FilterSubType::st_Standard}},
        {{Passband::LP, Slope::Slope_24dB, DriveMode::Standard},
         {sft::FilterType::fut_lp24, sft::FilterSubType::st_Standard}},
        {{Passband::LP, Slope::Slope_12dB, DriveMode::Clean},
         {sft::FilterType::fut_lp12, sft::FilterSubType::st_Clean}},
        {{Passband::LP, Slope::Slope_24dB, DriveMode::Clean},
         {sft::FilterType::fut_lp24, sft::FilterSubType::st_Clean}},
        {{Passband::LP, Slope::Slope_12dB, DriveMode::Driven},
         {sft::FilterType::fut_lp12, sft::FilterSubType::st_Driven}},
        {{Passband::LP, Slope::Slope_24dB, DriveMode::Driven},
         {sft::FilterType::fut_lp24, sft::FilterSubType::st_Driven}},

        {{Passband::HP, Slope::Slope_12dB, DriveMode::Standard},
         {sft::FilterType::fut_hp12, sft::FilterSubType::st_Standard}},
        {{Passband::HP, Slope::Slope_24dB, DriveMode::Standard},
         {sft::FilterType::fut_hp24, sft::FilterSubType::st_Standard}},
        {{Passband::HP, Slope::Slope_12dB, DriveMode::Clean},
         {sft::FilterType::fut_hp12, sft::FilterSubType::st_Clean}},
        {{Passband::HP, Slope::Slope_24dB, DriveMode::Clean},
         {sft::FilterType::fut_hp24, sft::FilterSubType::st_Clean}},
        {{Passband::HP, Slope::Slope_12dB, DriveMode::Driven},
         {sft::FilterType::fut_hp12, sft::FilterSubType::st_Driven}},
        {{Passband::HP, Slope::Slope_24dB, DriveMode::Driven},
         {sft::FilterType::fut_hp24, sft::FilterSubType::st_Driven}},

        {{Passband::BP, Slope::Slope_12dB, DriveMode::Standard},
         {sft::FilterType::fut_bp12, sft::FilterSubType::st_Standard}},
        {{Passband::BP, Slope::Slope_24dB, DriveMode::Standard},
         {sft::FilterType::fut_bp24, sft::FilterSubType::st_Standard}},
        {{Passband::BP, Slope::Slope_12dB, DriveMode::Clean},
         {sft::FilterType::fut_bp12, sft::FilterSubType::st_Clean}},
        {{Passband::BP, Slope::Slope_24dB, DriveMode::Clean},
         {sft::FilterType::fut_bp24, sft::FilterSubType::st_Clean}},
        {{Passband::BP, Slope::Slope_12dB, DriveMode::Driven},
         {sft::FilterType::fut_bp12, sft::FilterSubType::st_Driven}},
        {{Passband::BP, Slope::Slope_24dB, DriveMode::Driven},
         {sft::FilterType::fut_bp24, sft::FilterSubType::st_Driven}},

        {{Passband::Notch, Slope::Slope_12dB, DriveMode::Standard},
         {sft::FilterType::fut_notch12, sft::FilterSubType::st_Notch}},
        {{Passband::Notch, Slope::Slope_12dB, DriveMode::NotchMild},
         {sft::FilterType::fut_notch12, sft::FilterSubType::st_NotchMild}},
        {{Passband::Notch, Slope::Slope_24dB, DriveMode::Standard},
         {sft::FilterType::fut_notch24, sft::FilterSubType::st_Notch}},
        {{Passband::Notch, Slope::Slope_24dB, DriveMode::NotchMild},
         {sft::FilterType::fut_notch24, sft::FilterSubType::st_NotchMild}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::vemberclassic

#endif // VEMBERCLASSIC_H
