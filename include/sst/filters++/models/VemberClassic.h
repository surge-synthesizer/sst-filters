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
        {{PassTypes::LP, SlopeLevels::Slope_12db, DriveTypes::Standard},
         {sft::FilterType::fut_lp12, sft::FilterSubType::st_Standard}},
        {{PassTypes::LP, SlopeLevels::Slope_24db, DriveTypes::Standard},
         {sft::FilterType::fut_lp24, sft::FilterSubType::st_Standard}},
        {{PassTypes::LP, SlopeLevels::Slope_12db, DriveTypes::Clean},
         {sft::FilterType::fut_lp12, sft::FilterSubType::st_Clean}},
        {{PassTypes::LP, SlopeLevels::Slope_24db, DriveTypes::Clean},
         {sft::FilterType::fut_lp24, sft::FilterSubType::st_Clean}},
        {{PassTypes::LP, SlopeLevels::Slope_12db, DriveTypes::Driven},
         {sft::FilterType::fut_lp12, sft::FilterSubType::st_Driven}},
        {{PassTypes::LP, SlopeLevels::Slope_24db, DriveTypes::Driven},
         {sft::FilterType::fut_lp24, sft::FilterSubType::st_Driven}},

        {{PassTypes::HP, SlopeLevels::Slope_12db, DriveTypes::Standard},
         {sft::FilterType::fut_hp12, sft::FilterSubType::st_Standard}},
        {{PassTypes::HP, SlopeLevels::Slope_24db, DriveTypes::Standard},
         {sft::FilterType::fut_hp24, sft::FilterSubType::st_Standard}},
        {{PassTypes::HP, SlopeLevels::Slope_12db, DriveTypes::Clean},
         {sft::FilterType::fut_hp12, sft::FilterSubType::st_Clean}},
        {{PassTypes::HP, SlopeLevels::Slope_24db, DriveTypes::Clean},
         {sft::FilterType::fut_hp24, sft::FilterSubType::st_Clean}},
        {{PassTypes::HP, SlopeLevels::Slope_12db, DriveTypes::Driven},
         {sft::FilterType::fut_hp12, sft::FilterSubType::st_Driven}},
        {{PassTypes::HP, SlopeLevels::Slope_24db, DriveTypes::Driven},
         {sft::FilterType::fut_hp24, sft::FilterSubType::st_Driven}},

        {{PassTypes::BP, SlopeLevels::Slope_12db, DriveTypes::Standard},
         {sft::FilterType::fut_bp12, sft::FilterSubType::st_Standard}},
        {{PassTypes::BP, SlopeLevels::Slope_24db, DriveTypes::Standard},
         {sft::FilterType::fut_bp24, sft::FilterSubType::st_Standard}},
        {{PassTypes::BP, SlopeLevels::Slope_12db, DriveTypes::Clean},
         {sft::FilterType::fut_bp12, sft::FilterSubType::st_Clean}},
        {{PassTypes::BP, SlopeLevels::Slope_24db, DriveTypes::Clean},
         {sft::FilterType::fut_bp24, sft::FilterSubType::st_Clean}},
        {{PassTypes::BP, SlopeLevels::Slope_12db, DriveTypes::Driven},
         {sft::FilterType::fut_bp12, sft::FilterSubType::st_Driven}},
        {{PassTypes::BP, SlopeLevels::Slope_24db, DriveTypes::Driven},
         {sft::FilterType::fut_bp24, sft::FilterSubType::st_Driven}},

        {{PassTypes::Notch, SlopeLevels::Slope_12db, DriveTypes::Standard},
         {sft::FilterType::fut_notch12, sft::FilterSubType::st_Notch}},
        {{PassTypes::Notch, SlopeLevels::Slope_12db, DriveTypes::NotchMild},
         {sft::FilterType::fut_notch12, sft::FilterSubType::st_NotchMild}},
        {{PassTypes::Notch, SlopeLevels::Slope_24db, DriveTypes::Standard},
         {sft::FilterType::fut_notch24, sft::FilterSubType::st_Notch}},
        {{PassTypes::Notch, SlopeLevels::Slope_24db, DriveTypes::NotchMild},
         {sft::FilterType::fut_notch24, sft::FilterSubType::st_NotchMild}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::vemberclassic

#endif // VEMBERCLASSIC_H
