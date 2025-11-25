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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_VINTAGELADDER_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_VINTAGELADDER_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::vintageladder
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Passband::LP, FilterSubModel::RungeKutta},
         {sft::FilterType::fut_vintageladder, sft::FilterSubType::st_vintage_type1}},
        {{Passband::LP, FilterSubModel::RungeKuttaCompensated},
         {sft::FilterType::fut_vintageladder, sft::FilterSubType::st_vintage_type1_compensated}},
        {{Passband::LP, FilterSubModel::Huov},
         {sft::FilterType::fut_vintageladder, sft::FilterSubType::st_vintage_type2}},
        {{Passband::LP, FilterSubModel::HuovCompensated},
         {sft::FilterType::fut_vintageladder, sft::FilterSubType::st_vintage_type2_compensated}},
        {{Passband::LP, FilterSubModel::Huov2010},
         {sft::FilterType::fut_vintageladder, sft::FilterSubType::st_vintage_type3}},
        {{Passband::LP, FilterSubModel::HuovCompensated2010},
         {sft::FilterType::fut_vintageladder, sft::FilterSubType::st_vintage_type3_compensated}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::vintageladder

#endif // VINTAGELADDER_H
