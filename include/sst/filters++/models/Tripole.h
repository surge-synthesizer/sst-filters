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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_TRIPOLE_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_TRIPOLE_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::tripole
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Slope::Slope_1Stage, FilterSubModel::LowLowLow},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LLL1}},
        {{Slope::Slope_2Stage, FilterSubModel::LowLowLow},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LLL2}},
        {{Slope::Slope_3Stage, FilterSubModel::LowLowLow},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LLL3}},

        {{Slope::Slope_1Stage, FilterSubModel::LowHighLow},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LHL1}},
        {{Slope::Slope_2Stage, FilterSubModel::LowHighLow},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LHL2}},
        {{Slope::Slope_3Stage, FilterSubModel::LowHighLow},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LHL3}},

        {{Slope::Slope_1Stage, FilterSubModel::HighLowHigh},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HLH1}},
        {{Slope::Slope_2Stage, FilterSubModel::HighLowHigh},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HLH2}},
        {{Slope::Slope_3Stage, FilterSubModel::HighLowHigh},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HLH3}},

        {{Slope::Slope_1Stage, FilterSubModel::HighHighHigh},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HHH1}},
        {{Slope::Slope_2Stage, FilterSubModel::HighHighHigh},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HHH2}},
        {{Slope::Slope_3Stage, FilterSubModel::HighHighHigh},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HHH3}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::tripole

#endif // TRIPOLE_H
