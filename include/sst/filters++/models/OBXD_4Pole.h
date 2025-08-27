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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_OBXD_4POLE_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_OBXD_4POLE_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::obxd_4pole
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Passband::LP, Slope::Slope_6dB},
         {sft::FilterType::fut_obxd_4pole, sft::FilterSubType::st_obxd4pole_6dB}},
        {{Passband::LP, Slope::Slope_12dB},
         {sft::FilterType::fut_obxd_4pole, sft::FilterSubType::st_obxd4pole_12dB}},
        {{Passband::LP, Slope::Slope_18dB},
         {sft::FilterType::fut_obxd_4pole, sft::FilterSubType::st_obxd4pole_18dB}},
        {{Passband::LP, Slope::Slope_24dB},
         {sft::FilterType::fut_obxd_4pole, sft::FilterSubType::st_obxd4pole_24dB}},
        {{Passband::LP, Slope::Slope_24dB, FilterSubModel::BrokenOBXD4Pole24},
         {sft::FilterType::fut_obxd_4pole, sft::FilterSubType::st_obxd4pole_broken24dB}},

        {{Passband::LP, Slope::Slope_Morph},
         {sft::FilterType::fut_obxd_4pole, sft::FilterSubType::st_obxd4pole_morph}},
    };
    return configs;
}
} // namespace sst::filtersplusplus::models::obxd_4pole

#endif // OBXF_4POLE_H
