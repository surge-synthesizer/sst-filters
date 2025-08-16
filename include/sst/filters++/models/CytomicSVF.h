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
        {{PassTypes::LP}, {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_lp}},
        {{PassTypes::HP}, {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_hp}},
        {{PassTypes::BP}, {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_bp}},
        {{PassTypes::Notch},
         {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_notch}},
        {{PassTypes::Peak}, {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_peak}},
        {{PassTypes::AllPass},
         {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_all}},
        {{PassTypes::LP}, {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_lp}},
        {{PassTypes::LowShelf},
         {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_lowshelf}},
        {{PassTypes::HighShelf},
         {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_highhelf}},
        {{PassTypes::Bell}, {sft::FilterType::fut_cytomicsvf, sft::FilterSubType::st_cytomic_bell}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::cytomicsvf

#endif // CYTOMICSVF_H
