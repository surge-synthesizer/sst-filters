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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_K35_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_K35_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::k35
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{PassTypes::LP, DriveTypes::K35_None},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType(0)}},
        {{PassTypes::LP, DriveTypes::K35_Mild},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType(1)}},
        {{PassTypes::LP, DriveTypes::K35_Moderate},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType(2)}},
        {{PassTypes::LP, DriveTypes::K35_Heavy},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType(3)}},
        {{PassTypes::LP, DriveTypes::K35_Extreme},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType(4)}},

        {{PassTypes::HP, DriveTypes::K35_None},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType(0)}},
        {{PassTypes::HP, DriveTypes::K35_Mild},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType(1)}},
        {{PassTypes::HP, DriveTypes::K35_Moderate},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType(2)}},
        {{PassTypes::HP, DriveTypes::K35_Heavy},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType(3)}},
        {{PassTypes::HP, DriveTypes::K35_Extreme},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType(4)}},
    };
    return configs;
}
} // namespace sst::filtersplusplus::models::k35

#endif // K35_H
