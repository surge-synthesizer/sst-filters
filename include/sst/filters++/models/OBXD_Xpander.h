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
        {{Passband::LP, Slope::Slope_6dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_lp1}},
        {{Passband::LP, Slope::Slope_12dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_lp2}},
        {{Passband::LP, Slope::Slope_18dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_lp3}},
        {{Passband::LP, Slope::Slope_24dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_lp4}},

        {{Passband::HP, Slope::Slope_18dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_hp3}},
        {{Passband::HP, Slope::Slope_12dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_hp2}},
        {{Passband::HP, Slope::Slope_6dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_hp1}},

        {{Passband::BP, Slope::Slope_24dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_bp4}},
        {{Passband::BP, Slope::Slope_12dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_bp2}},
        {{Passband::Notch, Slope::Slope_12dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_n2}},
        {{Passband::Phaser, Slope::Slope_18dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_ph3}},

        {{Passband::BP, Slope::Slope_18dB6dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_hp3lp1}},
        {{Passband::BP, Slope::Slope_12dB6dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_hp2lp1}},

        {{Passband::NotchAndLP, Slope::Slope_12dB6dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_n2lp1}},
        {{Passband::PhaserAndLP, Slope::Slope_18dB6dB},
         {sft::FilterType::fut_obxd_xpander, sft::FilterSubType::st_obxdxpander_ph3lp1}},
    };
    return configs;
}
} // namespace sst::filtersplusplus::models::obxd_xpander

#endif // OBXF_4POLE_H
