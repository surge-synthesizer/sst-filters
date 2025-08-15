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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_VEMBERLADDER_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_VEMBERLADDER_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::vemberladder
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{PassTypes::LP, SlopeLevels::Slope_6db},
         {sft::FilterType::fut_lpmoog, sft::FilterSubType::st_lpmoog_6dB}},
        {{PassTypes::LP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_lpmoog, sft::FilterSubType::st_lpmoog_12dB}},
        {{PassTypes::LP, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_lpmoog, sft::FilterSubType::st_lpmoog_18dB}},
        {{PassTypes::LP, SlopeLevels::Slope_24db},
         {sft::FilterType::fut_lpmoog, sft::FilterSubType::st_lpmoog_24dB}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::vemberladder

#endif // VEMBERLADDER_H
