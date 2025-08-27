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
        {{Passband::LP, DriveMode::K35_None},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType::st_k35_none}},
        {{Passband::LP, DriveMode::K35_Mild},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType::st_k35_mild}},
        {{Passband::LP, DriveMode::K35_Moderate},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType::st_k35_moderate}},
        {{Passband::LP, DriveMode::K35_Heavy},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType::st_k35_heavy}},
        {{Passband::LP, DriveMode::K35_Extreme},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType::st_k35_extreme}},
        {{Passband::LP, DriveMode::K35_Continuous},
         {sft::FilterType::fut_k35_lp, sft::FilterSubType::st_k35_continuous}},

        {{Passband::HP, DriveMode::K35_None},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType::st_k35_none}},
        {{Passband::HP, DriveMode::K35_Mild},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType::st_k35_mild}},
        {{Passband::HP, DriveMode::K35_Moderate},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType::st_k35_moderate}},
        {{Passband::HP, DriveMode::K35_Heavy},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType::st_k35_heavy}},
        {{Passband::HP, DriveMode::K35_Extreme},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType::st_k35_extreme}},
        {{Passband::HP, DriveMode::K35_Continuous},
         {sft::FilterType::fut_k35_hp, sft::FilterSubType::st_k35_continuous}},
    };
    return configs;
}
} // namespace sst::filtersplusplus::models::k35

#endif // K35_H
