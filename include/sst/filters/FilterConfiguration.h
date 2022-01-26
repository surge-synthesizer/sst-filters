#ifndef SST_FILTERS_FILTERCONFIGURATION_H
#define SST_FILTERS_FILTERCONFIGURATION_H

namespace sst::filters
{
/** These are the filter types we have to choose from! */
enum FilterType
{
    fut_none = 0,
    fut_lp12,
    fut_lp24,
    fut_lpmoog,
    fut_hp12,
    fut_hp24,
    fut_bp12,    // ADJ
    fut_notch12, // ADH
    fut_comb_pos,
    fut_SNH,
    fut_vintageladder,
    fut_obxd_2pole_lp, // ADJ
    fut_obxd_4pole,
    fut_k35_lp,
    fut_k35_hp,
    fut_diode,
    fut_cutoffwarp_lp,
    fut_cutoffwarp_hp,
    fut_cutoffwarp_n,
    fut_cutoffwarp_bp,
    fut_obxd_2pole_hp,
    fut_obxd_2pole_n,
    fut_obxd_2pole_bp,
    fut_bp24,
    fut_notch24,
    fut_comb_neg,
    fut_apf,
    fut_cutoffwarp_ap,
    fut_resonancewarp_lp,
    fut_resonancewarp_hp,
    fut_resonancewarp_n,
    fut_resonancewarp_bp,
    fut_resonancewarp_ap,
    fut_tripole,
    num_filter_types,
};

/*
 * Each filter needs w names (alas). there's the name we show in the automation parameter and
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
    /* this is a ruler to ensure names do not exceed 31 characters
     0123456789012345678901234567890
    */
};

const char fut_bp_subtypes[3][32] = {
    "Clean",
    "Driven",
    "Smooth",
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
    "Clean",
    "Driven",
    "Smooth",
};

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

const char fut_obxd_4p_subtypes[4][32] = {
    "6 dB",
    "12 dB",
    "18 dB",
    "24 dB",
};

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

const char fut_tripole_output_stage[4][16]{
    "First",
    "Second",
    "Third",
};

/** The number of sub-types for each filter type */
const int fut_subcount[num_filter_types] = {
    0,  // fut_none
    3,  // fut_lp12
    3,  // fut_lp24
    4,  // fut_lpmoog
    3,  // fut_hp12
    3,  // fut_hp24
    3,  // fut_bp12
    2,  // fut_notch12
    2,  // fut_comb_pos
    0,  // fut_SNH
    4,  // fut_vintageladder
    2,  // fut_obxd_2pole
    4,  // fut_obxd_4pole
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
};

enum FilterSubType
{
    st_SVF = 0,
    st_Rough = 1,
    st_Smooth = 2,
    st_Medium = 3,
    st_Notch = 0,
    st_NotchMild = 1,

    st_lpmoog_6dB = 0,
    st_lpmoog_12dB = 1,
    st_lpmoog_18dB = 2,
    st_lpmoog_24dB = 3,

    st_diode_6dB = 0,
    st_diode_12dB = 1,
    st_diode_18dB = 2,
    st_diode_24dB = 3,
};

} // namespace sst::filters

#endif // SST_FILTERS_FILTERCONFIGURATION_H
