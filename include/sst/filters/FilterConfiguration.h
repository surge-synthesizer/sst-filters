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
#ifndef INCLUDE_SST_FILTERS_FILTERCONFIGURATION_H
#define INCLUDE_SST_FILTERS_FILTERCONFIGURATION_H

#include <string>

namespace sst::filters
{
/** These are the filter types we have to choose from! */
enum FilterType
{
    fut_none = 0,         /**< Off */
    fut_lp12,             /**< Lowpass - 12 dB */
    fut_lp24,             /**< Lowpass - 24 dB */
    fut_lpmoog,           /**< Lowpass - Legacy Ladder */
    fut_hp12,             /**< Highpass - 12 dB */
    fut_hp24,             /**< Highpass - 24 dB */
    fut_bp12,             /**< Bandpass - 12 dB */
    fut_notch12,          /**< Notch - 12 dB */
    fut_comb_pos,         /**< Effect - Comb + */
    fut_SNH,              /**< Effect - Sample & Hold */
    fut_vintageladder,    /**< Lowpass - Vintage Ladder */
    fut_obxd_2pole_lp,    /**< Lowpass - OB-Xd 12 dB */
    fut_obxd_4pole,       /**< Lowpass - OB-Xd 24 dB */
    fut_k35_lp,           /**< Lowpass - K35 */
    fut_k35_hp,           /**< Highpass - K35 */
    fut_diode,            /**< Lowpass - Diode Ladder */
    fut_cutoffwarp_lp,    /**< Lowpass - Cutoff Warp */
    fut_cutoffwarp_hp,    /**< Higghpass - Cutoff Warp */
    fut_cutoffwarp_n,     /**< Notch - Cutoff Warp */
    fut_cutoffwarp_bp,    /**< Bandpass - Cutoff Warp */
    fut_obxd_2pole_hp,    /**< Highpass - OB-Xd 12 dB */
    fut_obxd_2pole_n,     /**< Notch - OB-Xd 12 dB */
    fut_obxd_2pole_bp,    /**< Bandpass - OB-Xd 12 dB */
    fut_bp24,             /**< Bandpass - 24 dB */
    fut_notch24,          /**< Notch - 24 dB */
    fut_comb_neg,         /**< Effect - Comb - */
    fut_apf,              /**< Effect - Allpass */
    fut_cutoffwarp_ap,    /**< Effect - Cutoff Warp Allpass */
    fut_resonancewarp_lp, /**< Lowpass - Resonance Warp */
    fut_resonancewarp_hp, /**< Highpass - Resonance Warp */
    fut_resonancewarp_n,  /**< Notch - Resonance Warp */
    fut_resonancewarp_bp, /**< Bandpass - Resonance Warp */
    fut_resonancewarp_ap, /**< Effect - Resonance Warp Allpass */
    fut_tripole,          /**< Multi - Tri-pole */
    fut_cytomic_svf,      /**< Multi - Cytomic SVF */
    fut_obxd_xpander,     /**< Multi - OB-Xd Xpander */
    num_filter_types,
};

/*
 * Each filter needs names (alas). there's the name we show in the automation parameter and
 * so on (the value for get_display_name) which is in 'fut_names'. There's the value we put
 * in the menu which generally strips out Lowpass and Highpass and stuff, since they are already
 * grouped in submenus, and this is in fut_menu array
 */
const char filter_type_names[num_filter_types][32] = {
    "Off",               // fut_none
    "LP 12 dB",          // fut_lp12
    "LP 24 dB",          // fut_lp24
    "LP Legacy Ladder",  // fut_lpmoog
    "HP 12 dB",          // fut_hp12
    "HP 24 dB",          // fut_hp24
    "BP 12 dB",          // fut_bp12
    "N 12 dB",           // fut_notch12
    "FX Comb +",         // fut_comb_pos
    "FX Sample & Hold",  // fut_SNH
    "LP Vintage Ladder", // fut_vintageladder
    "LP OB-Xd 12 dB",    // fut_obxd_2pole_lp
    "LP OB-Xd 24 dB",    // fut_obxd_4pole
    "LP K35",            // fut_k35_lp
    "HP K35",            // fut_k35_hp
    "LP Diode Ladder",   // fut_diode
    "LP Cutoff Warp",    // fut_cutoffwarp_lp
    "HP Cutoff Warp",    // fut_cutoffwarp_hp
    "N Cutoff Warp",     // fut_cutoffwarp_n
    "BP Cutoff Warp",    // fut_cutoffwarp_bp
    "HP OB-Xd 12 dB",    // fut_obxd_2pole_hp
    "N OB-Xd 12 dB",     // fut_obxd_2pole_n
    "BP OB-Xd 12 dB",    // fut_obxd_2pole_bp
    "BP 24 dB",          // fut_bp24
    "N 24 dB",           // fut_notch24
    "FX Comb -",         // fut_comb_neg
    "FX Allpass",        // fut_apf
    "FX Cutoff Warp AP", // fut_cutoffwarp_ap
    "LP Res Warp",       // fut_resonancewarp_lp
    "HP Res Warp",       // fut_resonancewarp_hp
    "N Res Warp",        // fut_resonancewarp_n
    "BP Res Warp",       // fut_resonancewarp_bp
    "FX Res Warp AP",    // fut_resonancewarp_ap
    "MULTI Tri-pole",    // fut_tripole
    "MULTI Fast SVF",    // fut_cytomic_svf
    "MULTI Xpander"      // fut_obxd_xpander
    /* this is a ruler to ensure names do not exceed 31 characters
     0123456789012345678901234567890
    */
};

const char filter_menu_names[num_filter_types][32] = {
    "Off",
    "12 dB", // LP
    "24 dB", // LP
    "Legacy Ladder",
    "12 dB", // HP
    "24 dB", // HP
    "12 dB", // BP
    "12 dB", // N
    "Comb +",
    "Sample & Hold",
    "Vintage Ladder",
    "OB-Xd 12 dB", // LP
    "OB-Xd 24 dB", // LP
    "K35",         // LP
    "K35",         // HP
    "Diode Ladder",
    "Cutoff Warp", // LP
    "Cutoff Warp", // HP
    "Cutoff Warp", // N
    "Cutoff Warp", // BP
    "OB-Xd 12 dB", // HP
    "OB-Xd 12 dB", // N
    "OB-Xd 12 dB", // BP
    "24 dB",       // BP
    "24 dB",       // N
    "Comb -",
    "Allpass",
    "Cutoff Warp Allpass",
    "Resonance Warp", // LP
    "Resonance Warp", // HP
    "Resonance Warp", // N
    "Resonance Warp", // BP
    "Resonance Warp Allpass",
    "Tri-pole",
    "Fast SVF",
    "Xpander"
    /* this is a ruler to ensure names do not exceed 31 characters
     0123456789012345678901234567890
    */
};

const char fut_notch_subtypes[2][32] = {
    "Standard",
    "Mild",
};

const char fut_comb_subtypes[2][64] = {
    "50% Wet",
    "100% Wet",
};

const char fut_def_subtypes[3][32] = {
    "Standard",
    "Driven",
    "Clean",
};

const char fut_bp12_subtypes[5][32] = {"Standard", "Driven", "Clean", "Driven (Legacy)",
                                       "Clean (Legacy)"};

const char fut_ldr_subtypes[4][32] = {
    "6 dB",
    "12 dB",
    "18 dB",
    "24 dB",
};

const char fut_vintageladder_subtypes[6][32] = {
    "Type 1",
    "Type 1 Compensated",
    "Type 2",
    "Type 2 Compensated",
};

const char fut_obxd_2p_subtypes[2][32] = {"Standard", "Pushed"};

const char fut_obxd_4p_subtypes[6][32] = {"6 dB",  "12 dB",          "18 dB",
                                          "24 dB", "24 dB (Legacy)", "Morph"};

const char fut_k35_subtypes[5][32] = {"No Saturation", "Mild Saturation", "Moderate Saturation",
                                      "Heavy Saturation", "Extreme Saturation"};

const float fut_k35_saturations[5] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};

const char fut_nlf_subtypes[4][32] = {
    "1 Stage",
    "2 Stages",
    "3 Stages",
    "4 Stages",
};

const char fut_nlf_saturators[4][16] = {
    "tanh",
    "Soft Clip",
    "OJD",
    "Sine",
};

const char fut_tripole_subtypes[4][32] = {
    "Low -> Low -> Low",
    "Low -> High -> Low",
    "High -> Low -> High",
    "High -> High -> High",
};

const char fut_tripole_output_stage[3][16]{
    "First",
    "Second",
    "Third",
};

const char fut_cytomic_subtypes[9][32] = {"LP",      "HP",   "BP",        "Notch",     "Peak",
                                          "Allpass", "Bell", "Low Shelf", "High Shelf"};

const char fut_obxd_xpander_subtypes[15][32] = {"LP1", "LP2",     "LP3",     "LP4",    "HP1",
                                                "HP2", "HP3",     "BP2",     "BP4",    "N2",
                                                "PH3", "HP2+LP1", "HP3+LP1", "N2+LP1", "PH3+LP1"};

/** The number of sub-types for each filter type */
const int fut_subcount[num_filter_types] = {
    0,  // fut_none
    3,  // fut_lp12
    3,  // fut_lp24
    4,  // fut_lpmoog
    3,  // fut_hp12
    3,  // fut_hp24
    5,  // fut_bp12
    2,  // fut_notch12
    2,  // fut_comb_pos (excluding morph)
    0,  // fut_SNH
    6,  // fut_vintageladder
    2,  // fut_obxd_2pole
    5,  // fut_obxd_4pole (excluding morph)
    5,  // fut_k35_lp
    5,  // fut_k35_hp
    4,  // fut_diode
    12, // fut_cutoffwarp_lp
    12, // fut_cutoffwarp_hp
    12, // fut_cutoffwarp_n
    12, // fut_cutoffwarp_bp
    2,  // fut_obxd_2pole_hp,
    2,  // fut_obxd_2pole_n,
    2,  // fut_obxd_2pole_bp,
    3,  // fut_bp24,
    2,  // fut_notch24,
    2,  // fut_comb_neg,
    0,  // fut_apf
    12, // fut_cutoffwarp_ap
    8,  // fut_resonancewarp_lp
    8,  // fut_resonancewarp_hp
    8,  // fut_resonancewarp_n
    8,  // fut_resonancewarp_bp
    8,  // fut_resonancewarp_ap
    12, // fut_tripole
    9,  // fut_cytomic
    15  // fit_obxd_xpander
};

/*
 * Subtypes are integers below 16 - maybe one day go as high as 32. So we have space in the
 * int for more information, and we mask on higher bits to allow us to
 * programmatically change features we don't expose to users, in things like
 * FX. So far this is only used to extend the COMB time in the combulator.
 *
 * These should obviously be distinct per type but can overlap in values otherwise
 *
 * Try and use above 2^16
 */
enum QFUSubtypeMasks : int32_t
{
    UNMASK_SUBTYPE = (1 << 8) - 1,
    EXTENDED_COMB = 1 << 9
};

/** Sub-types for each filter are defined here */
enum FilterSubType
{
    st_Standard = 0, /**< Standard (SVF) */
    st_Driven = 1,   /**< Driven */
    st_Clean = 2,    /**< Clean */
    st_Medium = 3,   /**< (Unused) */

    st_Notch = 0,     /**< Standard */
    st_NotchMild = 1, /**< Mild */

    st_lpmoog_6dB = 0,  /**< Legacy Ladder - 6 dB */
    st_lpmoog_12dB = 1, /**< Legacy Ladder - 12 dB */
    st_lpmoog_18dB = 2, /**< Legacy Ladder - 18 dB */
    st_lpmoog_24dB = 3, /**< Legacy Ladder - 24 dB */

    st_diode_6dB = 0,  /**< Diode Ladder - 6 dB */
    st_diode_12dB = 1, /**< Diode Ladder - 12 dB */
    st_diode_18dB = 2, /**< Diode Ladder - 18 dB */
    st_diode_24dB = 3, /**< Diode Ladder - 24 dB */

    st_cutoffwarp_tanh1 = 0,     /**< Cutoff Warp - 1 Stage tanh */
    st_cutoffwarp_tanh2 = 1,     /**< Cutoff Warp - 2 Stages tanh */
    st_cutoffwarp_tanh3 = 2,     /**< Cutoff Warp - 3 Stages tanh */
    st_cutoffwarp_tanh4 = 3,     /**< Cutoff Warp - 4 Stages tanh */
    st_cutoffwarp_softclip1 = 4, /**< Cutoff Warp - 1 Stage Soft Clip */
    st_cutoffwarp_softclip2 = 5, /**< Cutoff Warp - 2 Stages Soft Clip */
    st_cutoffwarp_softclip3 = 6, /**< Cutoff Warp - 3 Stages Soft Clip */
    st_cutoffwarp_softclip4 = 7, /**< Cutoff Warp - 4 Stages Soft Clip */
    st_cutoffwarp_ojd1 = 8,      /**< Cutoff Warp - 1 Stage OJD */
    st_cutoffwarp_ojd2 = 9,      /**< Cutoff Warp - 2 Stages OJD */
    st_cutoffwarp_ojd3 = 10,     /**< Cutoff Warp - 3 Stages OJD */
    st_cutoffwarp_ojd4 = 11,     /**< Cutoff Warp - 4 Stages OJD */

    st_vintage_type1 = 0,             /**< Vintage Ladder - Runge Kutta  */
    st_vintage_type1_compensated = 1, /**< and with bass compensation */
    st_vintage_type2 = 2,             /**< Vintage Ladder - Huovilean Model */
    st_vintage_type2_compensated = 3, /**< and with bass compensation */
    st_vintage_type3 = 4,             /**< Vintage Ladder - Huovilean 2010 Model */
    st_vintage_type3_compensated = 5, /**< and with bass compensation */

    st_resonancewarp_tanh1 = 0,     /**< Resonance Warp - 1 Stage tanh */
    st_resonancewarp_tanh2 = 1,     /**< Resonance Warp - 2 Stages tanh */
    st_resonancewarp_tanh3 = 2,     /**< Resonance Warp - 3 Stages tanh */
    st_resonancewarp_tanh4 = 3,     /**< Resonance Warp - 4 Stages tanh */
    st_resonancewarp_softclip1 = 4, /**< Resonance Warp - 1 Stage Soft Clip */
    st_resonancewarp_softclip2 = 5, /**< Resonance Warp - 2 Stages Soft Clip */
    st_resonancewarp_softclip3 = 6, /**< Resonance Warp - 3 Stages Soft Clip */
    st_resonancewarp_softclip4 = 7, /**< Resonance Warp - 4 Stages Soft Clip */

    st_tripole_LLL1 = 0,  /**< Low -> Low -> Low, first stage output */
    st_tripole_LHL1 = 1,  /**< Low -> High -> Low, first stage output */
    st_tripole_HLH1 = 2,  /**< High -> Low -> High, first stage output */
    st_tripole_HHH1 = 3,  /**< High -> High -> High, first stage output */
    st_tripole_LLL2 = 4,  /**< Low -> Low -> Low, second stage output */
    st_tripole_LHL2 = 5,  /**< Low -> High -> Low, second stage output */
    st_tripole_HLH2 = 6,  /**< High -> Low -> High, second stage output */
    st_tripole_HHH2 = 7,  /**< High -> High -> High, second stage output */
    st_tripole_LLL3 = 8,  /**< Low -> Low -> Low, third stage output */
    st_tripole_LHL3 = 9,  /**< Low -> High -> Low, third stage output */
    st_tripole_HLH3 = 10, /**< High -> Low -> High, third stage output */
    st_tripole_HHH3 = 11, /**< High -> High -> High, third stage output */

    // Surge 1.x has a broken 24dB. This retains it.
    st_obxd4pole_6dB = 0,
    st_obxd4pole_12dB = 1,
    st_obxd4pole_18dB = 2,
    st_obxd4pole_24dB = 3,
    st_obxd4pole_broken24dB = 4,
    st_obxd4pole_morph = 5, // this is a sentinel value to use extra2

    st_obxd2pole_standard = 0,
    st_obxd2pole_pushed = 1,

    // Comb is special. Its the same CME and 0123 and 50/100/50/100 but for pos neg so
    st_comb_pos_50 = 0,
    st_comb_pos_100 = 1,
    st_comb_neg_50 = 2,
    st_comb_neg_100 = 3,
    st_comb_continuous_pos = 4,    // this works with pos
    st_comb_continuous_neg = 5,    // this works with pos
    st_comb_continuous_posneg = 6, // this works with pos

    // Add these to the enum so that asan doesn't complain when we cast them basically
    st_comb_pos_50_ext = st_comb_pos_50 | QFUSubtypeMasks::EXTENDED_COMB,
    st_comb_pos_100_ext = st_comb_pos_100 | QFUSubtypeMasks::EXTENDED_COMB,
    st_comb_neg_50_ext = st_comb_neg_50 | QFUSubtypeMasks::EXTENDED_COMB,
    st_comb_neg_100_ext = st_comb_neg_100 | QFUSubtypeMasks::EXTENDED_COMB,

    // Legacy fixes for BP12 misconfiguration
    st_bp12_LegacyDriven = 3,
    st_bp12_LegacyClean = 4,

    // K35
    st_k35_none = 0,
    st_k35_mild = 1,
    st_k35_moderate = 2,
    st_k35_heavy = 3,
    st_k35_extreme = 4,
    st_k35_continuous = 5,

    // Cytomic has the passtypes as subtypes
    st_cytomic_lp = 0,
    st_cytomic_hp = 1,
    st_cytomic_bp = 2,
    st_cytomic_notch = 3,
    st_cytomic_peak = 4,
    st_cytomic_allpass = 5,
    st_cytomic_bell = 6, // these three use "extra1"
    st_cytomic_lowshelf = 7,
    st_cytomic_highshelf = 8,

    st_obxdxpander_lp1 = 0,
    st_obxdxpander_lp2 = 1,
    st_obxdxpander_lp3 = 2,
    st_obxdxpander_lp4 = 3,
    st_obxdxpander_hp1 = 4,
    st_obxdxpander_hp2 = 5,
    st_obxdxpander_hp3 = 6,
    st_obxdxpander_bp2 = 7,
    st_obxdxpander_bp4 = 8,
    st_obxdxpander_n2 = 9,
    st_obxdxpander_ph3 = 10,
    st_obxdxpander_hp2lp1 = 11,
    st_obxdxpander_hp3lp1 = 12,
    st_obxdxpander_n2lp1 = 13,
    st_obxdxpander_ph3lp1 = 14
};

} // namespace sst::filters

#endif // SST_FILTERS_FILTERCONFIGURATION_H
