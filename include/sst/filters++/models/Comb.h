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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_COMB_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_COMB_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::comb
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{SlopeLevels::Comb_Positive_50}, {sft::FilterType::fut_comb_pos, sft::FilterSubType(0)}},
        {{SlopeLevels::Comb_Positive_100}, {sft::FilterType::fut_comb_pos, sft::FilterSubType(1)}},
        {{SlopeLevels::Comb_Negative_50}, {sft::FilterType::fut_comb_neg, sft::FilterSubType(0)}},
        {{SlopeLevels::Comb_Negative_100}, {sft::FilterType::fut_comb_neg, sft::FilterSubType(1)}},
        {{SlopeLevels::Comb_Positive_ContinuousMix},
         {sft::FilterType::fut_comb_pos, sft::st_comb_continuous_pos}},
        {{SlopeLevels::Comb_Negative_ContinuousMix},
         {sft::FilterType::fut_comb_pos, sft::st_comb_continuous_neg}},
        {{SlopeLevels::Comb_Bipolar_ContinuousMix},
         {sft::FilterType::fut_comb_pos, sft::st_comb_continuous_posneg}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::comb

#endif // COMB_H
