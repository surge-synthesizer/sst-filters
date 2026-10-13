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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_CYTOMICCASCADE_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_CYTOMICCASCADE_H

#include "sst/filters.h"

namespace sst::filtersplusplus::models::cytomiccascade
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs = []() {
        details::FilterPayload::configMap_t res;
        const std::pair<Passband, sft::FilterType> passbands[6] = {
            {Passband::LP, sft::fut_cytomic_cascade_lp},
            {Passband::HP, sft::fut_cytomic_cascade_hp},
            {Passband::BP, sft::fut_cytomic_cascade_bp},
            {Passband::Notch, sft::fut_cytomic_cascade_n},
            {Passband::Peak, sft::fut_cytomic_cascade_pk},
            {Passband::Allpass, sft::fut_cytomic_cascade_ap}};
        for (auto [pb, ft] : passbands)
        {
            res[{pb, Slope::Slope_24dB, DriveMode::Standard}] = {ft, sft::st_cascade_standard};
            res[{pb, Slope::Slope_24dB, DriveMode::Driven}] = {ft, sft::st_cascade_driven};
        }
        res[{Passband::LP, Slope::Slope_24dB, DriveMode::Standard, FilterSubModel::Compensated}] = {
            sft::fut_cytomic_cascade_lp, sft::st_cascade_compensated};
        res[{Passband::LP, Slope::Slope_24dB, DriveMode::Driven, FilterSubModel::Compensated}] = {
            sft::fut_cytomic_cascade_lp, sft::st_cascade_driven_compensated};
        return res;
    }();
    return configs;
}
} // namespace sst::filtersplusplus::models::cytomiccascade

#endif // INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_CYTOMICCASCADE_H
