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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_RESONANCEWARP_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_RESONANCEWARP_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::resonancewarp
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{};

    if (configs.empty())
    {
        // This one is too tedious to not fill programatically
        for (auto [pt, qft] : {std::make_pair(Passband::LP, sft::FilterType::fut_resonancewarp_lp),
                               {Passband::HP, sft::FilterType::fut_resonancewarp_hp},
                               {Passband::BP, sft::FilterType::fut_resonancewarp_bp},
                               {Passband::Notch, sft::FilterType::fut_resonancewarp_n},
                               {Passband::Allpass, sft::FilterType::fut_resonancewarp_ap}})
        {
            for (auto [dt, off] : {std::make_pair(DriveMode::Tanh, 0), {DriveMode::SoftClip, 4}})
            {
                auto idx{0};
                for (auto sm : {FilterSubModel::Warp_1Stage, FilterSubModel::Warp_2Stage,
                                FilterSubModel::Warp_3Stage, FilterSubModel::Warp_4Stage})
                {
                    auto key = details::FilterPayload::modelConfig_t{pt, dt, sm};
                    auto fv =
                        (uint32_t)sst::filters::FilterSubType::st_resonancewarp_tanh1 + off + idx;
                    ;

                    auto old = std::make_pair(qft, (sst::filters::FilterSubType)fv);
                    configs[key] = old;
                    idx++;
                }
            }
        }
    }
    return configs;
}
} // namespace sst::filtersplusplus::models::resonancewarp

#endif // RESONANCEWARP_H
