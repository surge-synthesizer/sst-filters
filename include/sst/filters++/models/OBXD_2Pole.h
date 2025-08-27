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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_OBXD_2POLE_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_MODELS_OBXD_2POLE_H

#include "sst/filters.h"

#include <unordered_set>

namespace sst::filtersplusplus::models::obxd_2pole
{
inline const details::FilterPayload::configMap_t &getModelConfigurations()
{
    namespace sft = sst::filters;
    static details::FilterPayload::configMap_t configs{
        {{Passband::LP, DriveMode::Standard},
         {sft::FilterType::fut_obxd_2pole_lp, sft::FilterSubType::st_obxd2pole_standard}},
        {{Passband::LP, DriveMode::Pushed},
         {sft::FilterType::fut_obxd_2pole_lp, sft::FilterSubType::st_obxd2pole_pushed}},

        {{Passband::HP, DriveMode::Standard},
         {sft::FilterType::fut_obxd_2pole_hp, sft::FilterSubType::st_obxd2pole_standard}},
        {{Passband::HP, DriveMode::Pushed},
         {sft::FilterType::fut_obxd_2pole_hp, sft::FilterSubType::st_obxd2pole_pushed}},

        {{Passband::BP, DriveMode::Standard},
         {sft::FilterType::fut_obxd_2pole_bp, sft::FilterSubType::st_obxd2pole_standard}},
        {{Passband::BP, DriveMode::Pushed},
         {sft::FilterType::fut_obxd_2pole_bp, sft::FilterSubType::st_obxd2pole_pushed}},

        {{Passband::Notch, DriveMode::Standard},
         {sft::FilterType::fut_obxd_2pole_n, sft::FilterSubType::st_obxd2pole_standard}},
        {{Passband::Notch, DriveMode::Pushed},
         {sft::FilterType::fut_obxd_2pole_n, sft::FilterSubType::st_obxd2pole_pushed}},

    };
    return configs;
}
} // namespace sst::filtersplusplus::models::obxd_2pole

#endif // OBXF_2POLE_H
