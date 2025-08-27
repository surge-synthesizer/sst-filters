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

#ifndef INCLUDE_SST_FILTERS_FILTERCONFIGURATIONLABELS_H
#define INCLUDE_SST_FILTERS_FILTERCONFIGURATIONLABELS_H

#include <string>
#include "FilterConfiguration.h"

namespace sst::filters
{

static std::string subtypeLabel(int type, int subtype)
{
    using sst::filters::FilterType;
    int i = subtype;
    const auto fType = (FilterType)type;
    if (sst::filters::fut_subcount[type] == 0)
    {
        return "None";
    }
    else
    {
        switch (fType)
        {
        case FilterType::fut_lpmoog:
        case FilterType::fut_diode:
            return sst::filters::fut_ldr_subtypes[i];
            break;
        case FilterType::fut_notch12:
        case FilterType::fut_notch24:
        case FilterType::fut_apf:
            return sst::filters::fut_notch_subtypes[i];
            break;
        case FilterType::fut_comb_pos:
        case FilterType::fut_comb_neg:
            return sst::filters::fut_comb_subtypes[i];
            break;
        case FilterType::fut_vintageladder:
            return sst::filters::fut_vintageladder_subtypes[i];
            break;
        case FilterType::fut_obxd_2pole_lp:
        case FilterType::fut_obxd_2pole_hp:
        case FilterType::fut_obxd_2pole_n:
        case FilterType::fut_obxd_2pole_bp:
            return sst::filters::fut_obxd_2p_subtypes[i];
            break;
        case FilterType::fut_obxd_4pole:
            return sst::filters::fut_obxd_4p_subtypes[i];
            break;
        case FilterType::fut_k35_lp:
        case FilterType::fut_k35_hp:
            return sst::filters::fut_k35_subtypes[i];
            break;
        case FilterType::fut_cutoffwarp_lp:
        case FilterType::fut_cutoffwarp_hp:
        case FilterType::fut_cutoffwarp_n:
        case FilterType::fut_cutoffwarp_bp:
        case FilterType::fut_cutoffwarp_ap:
        case FilterType::fut_resonancewarp_lp:
        case FilterType::fut_resonancewarp_hp:
        case FilterType::fut_resonancewarp_n:
        case FilterType::fut_resonancewarp_bp:
        case FilterType::fut_resonancewarp_ap:
            // "i & 3" selects the lower two bits that represent the stage count
            // "(i >> 2) & 3" selects the next two bits that represent the
            // saturator
            return fmt::format("{} {}", sst::filters::fut_nlf_subtypes[i & 3],
                               sst::filters::fut_nlf_saturators[(i >> 2) & 3]);
            break;
        // don't default any more so compiler catches new ones we add
        case FilterType::fut_none:
        case FilterType::fut_lp12:
        case FilterType::fut_lp24:
        case FilterType::fut_bp24:
        case FilterType::fut_hp12:
        case FilterType::fut_hp24:
        case FilterType::fut_SNH:
            return sst::filters::fut_def_subtypes[i];
            break;
        case FilterType::fut_bp12:
            return sst::filters::fut_bp12_subtypes[i];
            break;
        case FilterType::fut_tripole:
            // "i & 3" selects the lower two bits that represent the filter mode
            // "(i >> 2) & 3" selects the next two bits that represent the
            // output stage
            return fmt::format("{} {}", sst::filters::fut_tripole_subtypes[i & 3],
                               sst::filters::fut_tripole_output_stage[(i >> 2) & 3]);
            break;
        case FilterType::fut_cytomic_svf:
            return "t/k";
            break;
        case FilterType::fut_obxd_xpander:
            return "t/k";
            break;
        case FilterType::num_filter_types:
            return "ERROR";
            break;
        }
    }
    return "Error";
}

inline std::vector<std::pair<int, std::string>> filterGroupName()
{
    using sst::filters::FilterType;

    std::vector<std::pair<int, std::string>> res;
    auto p = [&res](auto a, auto b) { res.emplace_back(a, b); };

    p(FilterType::fut_none, "");

    p(FilterType::fut_lp12, "Lowpass");
    p(FilterType::fut_lp24, "Lowpass");
    p(FilterType::fut_lpmoog, "Lowpass");
    p(FilterType::fut_vintageladder, "Lowpass");
    p(FilterType::fut_k35_lp, "Lowpass");
    p(FilterType::fut_diode, "Lowpass");
    p(FilterType::fut_obxd_2pole_lp, "Lowpass"); // ADJ
    p(FilterType::fut_obxd_4pole, "Lowpass");
    p(FilterType::fut_cutoffwarp_lp, "Lowpass");
    p(FilterType::fut_resonancewarp_lp, "Lowpass");

    p(FilterType::fut_bp12, "Bandpass");
    p(FilterType::fut_bp24, "Bandpass");
    p(FilterType::fut_obxd_2pole_bp, "Bandpass");
    p(FilterType::fut_cutoffwarp_bp, "Bandpass");
    p(FilterType::fut_resonancewarp_bp, "Bandpass");

    p(FilterType::fut_hp12, "Highpass");
    p(FilterType::fut_hp24, "Highpass");
    p(FilterType::fut_k35_hp, "Highpass");
    p(FilterType::fut_obxd_2pole_hp, "Highpass");
    p(FilterType::fut_cutoffwarp_hp, "Highpass");
    p(FilterType::fut_resonancewarp_hp, "Highpass");

    p(FilterType::fut_notch12, "Notch");
    p(FilterType::fut_notch24, "Notch");
    p(FilterType::fut_obxd_2pole_n, "Notch");
    p(FilterType::fut_cutoffwarp_n, "Notch");
    p(FilterType::fut_resonancewarp_n, "Notch");

    p(FilterType::fut_tripole, "Multi");

    p(FilterType::fut_apf, "Effect");
    p(FilterType::fut_cutoffwarp_ap, "Effect");
    p(FilterType::fut_resonancewarp_ap, "Effect");
    p(FilterType::fut_comb_pos, "Effect");
    p(FilterType::fut_comb_neg, "Effect");
    p(FilterType::fut_SNH, "Effect");

    return res;
}

} // namespace sst::filters

#endif // SHORTCIRCUITXT_FILTERCONFIGURATIONLABELS_H
