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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_CYTOMICSVF_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_CYTOMICSVF_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::cytomicsvf
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Passband::LP}, {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_lp}},
        {{Passband::HP}, {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_hp}},
        {{Passband::BP}, {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_bp}},
        {{Passband::Notch},
         {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_notch}},
        {{Passband::Peak}, {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_peak}},
        {{Passband::Allpass},
         {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_allpass}},
        {{Passband::LP}, {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_lp}},
        {{Passband::LowShelf},
         {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_lowshelf}},
        {{Passband::HighShelf},
         {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_highshelf}},
        {{Passband::Bell}, {sft::FilterType::fut_cytomic_svf, sft::FilterSubType::st_cytomic_bell}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::cytomicsvf

#endif // CYTOMICSVF_H
