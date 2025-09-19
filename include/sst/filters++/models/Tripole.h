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
#include "sst/filters++/enums.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::tripole
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Passband::LowLowLow, FilterSubModel::First_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LLL1}},
        {{Passband::LowLowLow, FilterSubModel::Second_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LLL2}},
        {{Passband::LowLowLow, FilterSubModel::Third_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LLL3}},

        {{Passband::LowHighLow, FilterSubModel::First_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LHL1}},
        {{Passband::LowHighLow, FilterSubModel::Second_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LHL2}},
        {{Passband::LowHighLow, FilterSubModel::Third_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_LHL3}},

        {{Passband::HighLowHigh, FilterSubModel::First_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HLH1}},
        {{Passband::HighLowHigh, FilterSubModel::Second_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HLH2}},
        {{Passband::HighLowHigh, FilterSubModel::Third_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HLH3}},

        {{Passband::HighHighHigh, FilterSubModel::First_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HHH1}},
        {{Passband::HighHighHigh, FilterSubModel::Second_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HHH2}},
        {{Passband::HighHighHigh, FilterSubModel::Third_output},
         {sft::FilterType::fut_tripole, sft::FilterSubType::st_tripole_HHH3}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::tripole

#endif // TRIPOLE_H
