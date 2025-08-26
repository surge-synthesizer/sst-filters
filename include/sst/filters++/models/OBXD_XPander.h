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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_OBXD_XPANDER_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_OBXD_XPANDER_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::obxd_xpander
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{PassTypes::LP, SlopeLevels::Slope_6db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_lp1}},
        {{PassTypes::LP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_lp2}},
        {{PassTypes::LP, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_lp3}},
        {{PassTypes::LP, SlopeLevels::Slope_24db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_lp4}},

        {{PassTypes::HP, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_hp3}},
        {{PassTypes::HP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_hp2}},
        {{PassTypes::HP, SlopeLevels::Slope_6db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_hp1}},

        {{PassTypes::BP, SlopeLevels::Slope_24db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_bp4}},
        {{PassTypes::BP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_bp2}},
        {{PassTypes::Notch, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_n2}},
        {{PassTypes::Phaser, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_ph3}},

        {{PassTypes::HPAndLP, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_hp3lp1}},
        {{PassTypes::HPAndLP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_hp2lp1}},
        {{PassTypes::NotchAndLP, SlopeLevels::Slope_12db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_n2lp1}},
        {{PassTypes::PhaserAndLP, SlopeLevels::Slope_18db},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdX_ph3lp1}},
    };
    return configs;
}
} // namespace sst::filtersplusplus::models::obxd_xpander

#endif // OBXF_4POLE_H
