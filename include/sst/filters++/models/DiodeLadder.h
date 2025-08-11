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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_DIODELADDER_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_DIODELADDER_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::diodeladder
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{PassTypes::LP, SlopeLevels::Slope_6db},
         {sft::FilterType::fut_diode, sft::FilterSubType::st_diode_6dB}},
        {{PassTypes::LP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_diode, sft::FilterSubType::st_diode_12dB}},
        {{PassTypes::LP, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_diode, sft::FilterSubType::st_diode_18dB}},
        {{PassTypes::LP, SlopeLevels::Slope_24db},
         {sft::FilterType::fut_diode, sft::FilterSubType::st_diode_24dB}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::diodeladder

#endif // DIODELADDER_H
