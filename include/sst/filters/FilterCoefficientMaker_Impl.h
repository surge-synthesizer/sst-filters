#ifndef SST_FILTERS_FILTERCOEFFICIENTMAKER_IMPL_H
#define SST_FILTERS_FILTERCOEFFICIENTMAKER_IMPL_H

#include "FilterCoefficientMaker.h"
#include "sst/utilities/basic_dsp.h"
#include "sst/utilities/SincTable.h"
#include "QuadFilterUnit.h"

namespace sst::filters
{

// @TODO: In Surge this is implemented with a LUT. I think it's probably okay to use full precision.
template <typename T> inline T db_to_linear(T in) { return pow((T)10, (T)0.05 * in); }

constexpr float smooth = 0.2f;

template <typename TuningProvider> FilterCoefficientMaker<TuningProvider>::FilterCoefficientMaker()
{
    Reset();
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::setSampleRateAndBlockSize(float newSampleRate,
                                                                       int newBlockSize)
{
    sampleRate = newSampleRate;
    sampleRateInv = 1.0f / sampleRate;

    blockSize = newBlockSize;
    blockSizeInv = 1.0f / (float)blockSize;
}

namespace detail
{
inline void set1f(__m128 &m, int i, float f) { *((float *)&m + i) = f; }

inline float get1f(__m128 m, int i) { return *((float *)&m + i); }
} // namespace detail

template <typename TuningProvider>
template <typename StateType>
void FilterCoefficientMaker<TuningProvider>::updateState(StateType &state, int channel)
{
    if (channel < 0) // set for all channels
    {
        for (int i = 0; i < n_cm_coeffs; ++i)
        {
            state.C[i] = _mm_set1_ps(C[i]);
            state.dC[i] = _mm_set1_ps(dC[i]);
        }
    }
    else
    {
        for (int i = 0; i < n_cm_coeffs; ++i)
        {
            detail::set1f(state.C[i], channel, C[i]);
            detail::set1f(state.dC[i], channel, dC[i]);
        }
    }

    state.sampleRate = sampleRate;
    state.sampleRateInv = sampleRateInv;
}

template <typename TuningProvider>
template <typename StateType>
void FilterCoefficientMaker<TuningProvider>::updateCoefficients(StateType &state, int channel)
{
    for (int i = 0; i < n_cm_coeffs; i++)
        C[i] = detail::get1f(state.C[i], channel);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::MakeCoeffs(float Freq, float Reso, FilterType Type,
                                                        FilterSubType SubType,
                                                        TuningProvider *providerI,
                                                        bool tuningAdjusted)
{
    provider = providerI;
    if (provider)
    {
        if (tuningAdjusted && provider->tuningApplicationMode == TuningProvider::RETUNE_ALL)
        {
            /*
             * Modulations are not remapped and tuning is in effect; remap the note
             */
            auto idx = (int)floor(Freq + 69);
            float frac =
                (Freq + 69) - (float)idx; // frac is 0 means use idx; frac is 1 means use idx+1

            float b0 = (float)provider->currentTuning.logScaledFrequencyForMidiNote(idx) * 12.0f;
            float b1 =
                (float)provider->currentTuning.logScaledFrequencyForMidiNote(idx + 1) * 12.0f;

            auto q = (1.f - frac) * b0 + frac * b1;

            Freq = q - 69;
        }
    }

    switch (Type)
    {
    case fut_lp12:
        if (SubType == st_SVF)
            Coeff_SVF(Freq, Reso, false);
        else
            Coeff_LP12(Freq, Reso, SubType);
        break;
    case fut_hp12:
        if (SubType == st_SVF)
            Coeff_SVF(Freq, Reso, false);
        else
            Coeff_HP12(Freq, Reso, SubType);
        break;
    case fut_bp12:
        if (SubType == st_SVF)
            Coeff_SVF(Freq, Reso, false);
        else
            Coeff_BP12(Freq, Reso, SubType);
    case fut_bp24:
        if (SubType == st_SVF)
            Coeff_SVF(Freq, Reso, false); // WHY FALSE? It was this way before #3006 tho
        else
            Coeff_BP24(Freq, Reso, SubType);
        break;
    case fut_notch12:
    case fut_notch24:
        Coeff_Notch(Freq, Reso, SubType);
        break;
    case fut_apf:
        Coeff_APF(Freq, Reso, SubType);
        break;
    case fut_lp24:
        if (SubType == st_SVF)
            Coeff_SVF(Freq, Reso, true);
        else
            Coeff_LP24(Freq, Reso, SubType);
        break;
    case fut_hp24:
        if (SubType == st_SVF)
            Coeff_SVF(Freq, Reso, true);
        else
            Coeff_HP24(Freq, Reso, SubType);
        break;
    case fut_lpmoog:
        Coeff_LP4L(Freq, Reso, SubType);
        break;
    case fut_comb_pos:
        Coeff_COMB(Freq, Reso, SubType);
        break;
    case fut_comb_neg:
        Coeff_COMB(Freq, Reso, SubType + 2); // -ve feedback is the next 2 subtypes
        break;
    case fut_SNH:
        Coeff_SNH(Freq, Reso, SubType);
        break;
    case fut_vintageladder:
        switch (SubType)
        {
        case 0:
        case 1:
            VintageLadder::RK::makeCoefficients(this, Freq, Reso, sampleRate, SubType == 1,
                                                providerI);
            break;
        case 2:
        case 3:
            VintageLadder::Huov::makeCoefficients(this, Freq, Reso, sampleRate, sampleRateInv,
                                                  SubType == 3, providerI);
            break;
        default:
            // SOFTWARE ERROR
            break;
        }
        break;
    /**
     * When we split OBXD we went from one filter with 8 settings (L, B, H, N, L+, B+, H+, N+)
     * to two subtypes (regular and +). So what used to be Highpass (value 2 and 6) is now 1 and
     * 2 on a new type. But don't rewrite the filter for now. Just reconstruct the subtypes.
     */
    case fut_obxd_2pole_lp:
        OBXDFilter::makeCoefficients(this, OBXDFilter::TWO_POLE, Freq, Reso, SubType * 4,
                                     sampleRateInv, providerI);
        break;
    case fut_obxd_2pole_bp:
        OBXDFilter::makeCoefficients(this, OBXDFilter::TWO_POLE, Freq, Reso, SubType * 4 + 1,
                                     sampleRateInv, providerI);
        break;
    case fut_obxd_2pole_hp:
        OBXDFilter::makeCoefficients(this, OBXDFilter::TWO_POLE, Freq, Reso, SubType * 4 + 2,
                                     sampleRateInv, providerI);
        break;
    case fut_obxd_2pole_n:
        OBXDFilter::makeCoefficients(this, OBXDFilter::TWO_POLE, Freq, Reso, SubType * 4 + 3,
                                     sampleRateInv, providerI);
        break;
    case fut_obxd_4pole:
        OBXDFilter::makeCoefficients(this, OBXDFilter::FOUR_POLE, Freq, Reso, SubType,
                                     sampleRateInv, providerI);
        break;
    case fut_k35_lp:
        K35Filter::makeCoefficients(this, Freq, Reso, true, fut_k35_saturations[SubType],
                                    sampleRate, sampleRateInv, providerI);
        break;
    case fut_k35_hp:
        K35Filter::makeCoefficients(this, Freq, Reso, false, fut_k35_saturations[SubType],
                                    sampleRate, sampleRateInv, providerI);
        break;
    case fut_diode:
        DiodeLadderFilter::makeCoefficients(this, Freq, Reso, sampleRate, sampleRateInv, providerI);
        break;
    case fut_cutoffwarp_lp:
    case fut_cutoffwarp_hp:
    case fut_cutoffwarp_n:
    case fut_cutoffwarp_bp:
    case fut_cutoffwarp_ap:
        CutoffWarp::makeCoefficients(this, Freq, Reso, Type, SubType, sampleRate, providerI);
        break;
    case fut_resonancewarp_lp:
    case fut_resonancewarp_hp:
    case fut_resonancewarp_n:
    case fut_resonancewarp_bp:
    case fut_resonancewarp_ap:
        ResonanceWarp::makeCoefficients(this, Freq, Reso, Type, sampleRate, providerI);
        break;
    case fut_tripole:
        TriPoleFilter::makeCoefficients(this, Freq, Reso, Type, sampleRate, providerI);
        break;

    case num_filter_types:
        // This should really be an error condition of course
    case fut_none:
        break;
    }
}

inline float clipscale(float freq, int subtype)
{
    switch (subtype)
    {
    case st_Rough:
        return (1.0f / 64.0f) * db_to_linear(freq * 0.55f);
    case st_Smooth:
        return (1.0f / 1024.0f);
    default:
        return 0;
    }
}

[[nodiscard]] inline float boundFreq(float freq) { return std::clamp(freq, -55.0f, 75.0f); }

inline double Map2PoleResonance(double reso, double freq, int subtype)
{
    using std::max;
    switch (subtype)
    {
    case st_Medium:
        reso *= max(0.0, 1.0 - max(0.0, (freq - 58) * 0.05));
        return (0.99 -
                1.0 * utilities::limit_range((double)(1 - (1 - reso) * (1 - reso)), 0.0, 1.0));
    case st_Rough:
        reso *= max(0.0, 1.0 - max(0.0, (freq - 58) * 0.05));
        return (1.0 -
                1.05 * utilities::limit_range((double)(1 - (1 - reso) * (1 - reso)), 0.001, 1.0));
    default:
    case st_Smooth:
        return (2.5 -
                2.45 * utilities::limit_range((double)(1 - (1 - reso) * (1 - reso)), 0.0, 1.0));
    }
}

inline double Map2PoleResonance_noboost(double reso, double /*freq*/, int subtype)
{
    if (subtype == st_Rough)
        return (1.0 -
                0.99 * utilities::limit_range((double)(1 - (1 - reso) * (1 - reso)), 0.001, 1.0));
    else
        return (0.99 -
                0.98 * utilities::limit_range((double)(1 - (1 - reso) * (1 - reso)), 0.0, 1.0));
}

inline double Map4PoleResonance(double reso, double freq, int subtype)
{
    using std::max;
    switch (subtype)
    {
    case st_Medium:
        reso *= max(0.0, 1.0 - max(0.0, (freq - 58) * 0.05));
        return 0.99 - 0.9949 * utilities::limit_range((double)reso, 0.0, 1.0);
    case st_Rough:
        reso *= max(0.0, 1.0 - max(0.0, (freq - 58) * 0.05));
        return (1.0 - 1.05 * utilities::limit_range((double)reso, 0.001, 1.0));
    default:
    case st_Smooth:
        return (2.5 - 2.3 * utilities::limit_range((double)reso, 0.0, 1.0));
    }
}

template <typename T> inline T resoscale(T reso, int subtype)
{
    switch (subtype)
    {
    case st_Medium:
        return ((T)1.0 - (T)0.75 * reso * reso);
    case st_Rough:
        return ((T)1.0 - (T)0.5 * reso * reso);
    case st_Smooth:
        return ((T)1.0 - (T)0.25 * reso * reso);
    default:
        return (T)1.0;
    }
}

template <typename T> inline T resoscale4Pole(T reso, int subtype)
{
    switch (subtype)
    {
    case st_Medium:
        return ((T)1.0 - (T)0.75 * reso);
    case st_Rough:
        return ((T)1.0 - (T)0.5 * reso * reso);
    case st_Smooth:
        return ((T)1.0 - (T)0.5 * reso);
    default:
        return (T)1.0;
    }
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_SVF(float Freq, float Reso, bool FourPole)
{
    using std::min;

    double f = 440.f * provider->note_to_pitch_ignoring_tuning(Freq);
    double F1 = 2.0 * sin(M_PI * min(0.11, f * (0.5 * sampleRateInv))); // 2x oversampling

    Reso = sqrt(utilities::limit_range(Reso, 0.f, 1.f));

    double overshoot = FourPole ? 0.1 : 0.15;
    double Q1 = 2.0 - Reso * (2.0 + overshoot) + F1 * F1 * overshoot * 0.9;
    Q1 = min(Q1, min(2.00, 2.00 - 1.52 * F1));

    double ClipDamp = 0.1 * Reso * F1;

    const double a = 0.65;
    double Gain = 1 - a * Reso;

    float c[n_cm_coeffs]{};
    memset(c, 0, sizeof(float) * n_cm_coeffs);
    c[0] = (float)F1;
    c[1] = (float)Q1;
    c[2] = (float)ClipDamp;
    c[3] = (float)Gain;
    FromDirect(c);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_LP12(float freq, float reso, int subtype)
{
    float cosi, sinu;
    float gain = resoscale(reso, subtype);

    freq = boundFreq(freq);
    provider->note_to_omega_ignoring_tuning(freq, sinu, cosi, sampleRate);

    double alpha = sinu * Map2PoleResonance(reso, freq, subtype);

    if (subtype != st_Smooth)
    {
        alpha = std::min(alpha, sqrt(1.0 - cosi * cosi) - 0.0001);
    }

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0 = (1 - cosi) * 0.5,
           b1 = 1 - cosi, b2 = (1 - cosi) * 0.5;

    if (subtype == st_Smooth)
        ToNormalizedLattice(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain,
                            clipscale(freq, subtype));
    else
        ToCoupledForm(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain, clipscale(freq, subtype));
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_LP24(float freq, float reso, int subtype)
{
    float cosi, sinu;
    float gain = resoscale(reso, subtype);

    freq = boundFreq(freq);
    provider->note_to_omega_ignoring_tuning(freq, sinu, cosi, sampleRate);

    double Q2inv = Map4PoleResonance((double)reso, (double)freq, subtype);
    double alpha = sinu * Q2inv;

    if (subtype != st_Smooth)
    {
        alpha = std::min(alpha, sqrt(1.0 - cosi * cosi) - 0.0001);
    }

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0 = (1 - cosi) * 0.5,
           b1 = 1 - cosi, b2 = (1 - cosi) * 0.5;

    if (subtype == st_Smooth)
        ToNormalizedLattice(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain,
                            clipscale(freq, subtype));
    else
        ToCoupledForm(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain, clipscale(freq, subtype));
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_HP12(float freq, float reso, int subtype)
{
    float cosi, sinu;
    float gain = resoscale(reso, subtype);

    freq = boundFreq(freq);
    provider->note_to_omega_ignoring_tuning(freq, sinu, cosi, sampleRate);

    double Q2inv = Map2PoleResonance(reso, freq, subtype);
    double alpha = sinu * Q2inv;

    if (subtype != 0)
    {
        alpha = std::min(alpha, sqrt(1.0 - cosi * cosi) - 0.0001);
    }

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0 = (1 + cosi) * 0.5,
           b1 = -(1 + cosi), b2 = (1 + cosi) * 0.5;

    if (subtype == st_Smooth)
        ToNormalizedLattice(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain,
                            clipscale(freq, subtype));
    else
        ToCoupledForm(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain, clipscale(freq, subtype));
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_HP24(float freq, float reso, int subtype)
{
    float cosi, sinu;
    float gain = resoscale(reso, subtype);

    freq = boundFreq(freq);
    provider->note_to_omega_ignoring_tuning(freq, sinu, cosi, sampleRate);

    double Q2inv = Map4PoleResonance((double)reso, (double)freq, subtype);
    double alpha = sinu * Q2inv;

    if (subtype != 0)
    {
        alpha = std::min(alpha, sqrt(1.0 - cosi * cosi) - 0.0001);
    }

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0 = (1 + cosi) * 0.5,
           b1 = -(1 + cosi), b2 = (1 + cosi) * 0.5;

    if (subtype == st_Smooth)
        ToNormalizedLattice(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain,
                            clipscale(freq, subtype));
    else
        ToCoupledForm(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain, clipscale(freq, subtype));
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_BP12(float freq, float reso, int subtype)
{
    float cosi, sinu;
    float gain = resoscale(reso, subtype);

    if (subtype == st_Rough)
    {
        gain *= 2.f;
    }

    freq = boundFreq(freq);
    provider->note_to_omega_ignoring_tuning(freq, sinu, cosi, sampleRate);

    double Q2inv = Map2PoleResonance(reso, freq, subtype);
    double Q = 0.5 / Q2inv;
    double alpha = sinu * Q2inv;

    if (subtype != 0)
    {
        alpha = std::min(alpha, sqrt(1.0 - cosi * cosi) - 0.0001);
    }

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0, b1, b2;

    {
        b0 = Q * alpha;
        b1 = 0;
        b2 = -Q * alpha;
    }

    if (subtype == st_Smooth)
        ToNormalizedLattice(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain,
                            clipscale(freq, subtype));
    else
        ToCoupledForm(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain, clipscale(freq, subtype));
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_BP24(float freq, float reso, int subtype)
{
    float cosi, sinu;
    float gain = resoscale(reso, subtype);

    if (subtype == st_Rough)
    {
        gain *= 2.f;
    }

    freq = boundFreq(freq);
    provider->note_to_omega_ignoring_tuning(freq, sinu, cosi, sampleRate);

    double Q2inv = Map4PoleResonance(reso, freq, subtype);
    double Q = 0.5 / Q2inv;
    double alpha = sinu * Q2inv;

    if (subtype != 0)
    {
        alpha = std::min(alpha, sqrt(1.0 - cosi * cosi) - 0.0001);
    }

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0, b1, b2;

    {
        b0 = Q * alpha;
        b1 = 0;
        b2 = -Q * alpha;
    }

    if (subtype == st_Smooth)
        ToNormalizedLattice(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain,
                            clipscale(freq, subtype));
    else
        ToCoupledForm(a0inv, a1, a2, b0 * gain, b1 * gain, b2 * gain, clipscale(freq, subtype));
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_Notch(float Freq, float Reso, int SubType)
{
    float cosi, sinu;
    double Q2inv;

    Freq = boundFreq(Freq);
    provider->note_to_omega_ignoring_tuning(Freq, sinu, cosi, sampleRate);

    if (SubType == st_NotchMild)
    {
        Q2inv =
            (1.00 - 0.99 * utilities::limit_range((double)(1 - (1 - Reso) * (1 - Reso)), 0.0, 1.0));
    }
    else
    {
        Q2inv =
            (2.5 - 2.49 * utilities::limit_range((double)(1 - (1 - Reso) * (1 - Reso)), 0.0, 1.0));
    }

    double alpha = sinu * Q2inv;

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0 = 1, b1 = -2 * cosi,
           b2 = 1;

    ToNormalizedLattice(a0inv, a1, a2, b0, b1, b2, 0.005);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_APF(float Freq, float Reso, int /*SubType*/)
{
    float cosi, sinu;
    double Q2inv;

    Freq = boundFreq(Freq);
    provider->note_to_omega_ignoring_tuning(Freq, sinu, cosi, sampleRate);

    Q2inv = (2.5 - 2.49 * utilities::limit_range((double)(1 - (1 - Reso) * (1 - Reso)), 0.0, 1.0));

    double alpha = sinu * Q2inv;

    double a0 = 1 + alpha, a0inv = 1 / a0, a1 = -2 * cosi, a2 = 1 - alpha, b0 = 1 - alpha,
           b1 = -2 * cosi, b2 = 1 + alpha;

    ToNormalizedLattice(a0inv, a1, a2, b0, b1, b2, 0.005);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_LP4L(float freq, float reso, int /*subtype*/)
{
    double gg = utilities::limit_range(
        ((double)440 * provider->note_to_pitch_ignoring_tuning(freq) * (double)sampleRateInv), 0.0,
        0.187); // gg

    float t_b1 = 1.f - (float)exp(-2 * M_PI * gg);
    float q = std::min(2.15f * utilities::limit_range(reso, 0.f, 1.f),
                       0.5f / (t_b1 * t_b1 * t_b1 * t_b1));

    float c[n_cm_coeffs]{};
    memset(c, 0, sizeof(float) * n_cm_coeffs);
    c[0] = (3.f / (3.f - q));
    c[1] = t_b1;
    c[2] = q;

    FromDirect(c);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_COMB(float freq, float reso, int isubtype)
{
    int subtype = isubtype & QFUSubtypeMasks::UNMASK_SUBTYPE;
    bool extended = isubtype & QFUSubtypeMasks::EXTENDED_COMB;

    int comb_length = extended ? utilities::MAX_FB_COMB_EXTENDED : utilities::MAX_FB_COMB;

    float dtime = (1.f / 440.f) * provider->note_to_pitch_inv_ignoring_tuning(freq);
    dtime = dtime * sampleRate;

    // See comment in SurgeStorage and issue #3248
    if (provider != nullptr && !provider->_patch->correctlyTuneCombFilter)
    {
        dtime -= utilities::SincTable::FIRoffset;
    }

    dtime = utilities::limit_range(dtime, (float)utilities::SincTable::FIRipol_N,
                                   (float)comb_length - utilities::SincTable::FIRipol_N);
    if (extended)
    {
        // extended use is not from the filter bank so allow greater feedback range
        reso = utilities::limit_range(reso, -2.f, 2.f);
    }
    else
    {
        reso = ((subtype & 2) ? -1.0f : 1.0f) * utilities::limit_range(reso, 0.f, 1.f);
    }

    float c[n_cm_coeffs];
    memset(c, 0, sizeof(float) * n_cm_coeffs);
    c[0] = dtime;
    c[1] = reso;
    c[2] = (subtype & 1) ? 0.0f : 0.5f; // combmix
    c[3] = 1.f - c[2];
    FromDirect(c);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::Coeff_SNH(float freq, float reso, int /*subtype*/)
{
    float dtime = (1.f / 440.f) * provider->note_to_pitch_ignoring_tuning(-freq) * sampleRate;
    float v1 = 1.0f / dtime;

    float c[n_cm_coeffs]{};
    memset(c, 0, sizeof(float) * n_cm_coeffs);
    c[0] = v1;
    c[1] = reso;
    FromDirect(c);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::FromDirect(const float (&N)[n_cm_coeffs])
{
    if (FirstRun)
    {
        memset(dC, 0, sizeof(float) * n_cm_coeffs);
        memcpy(C, N, sizeof(float) * n_cm_coeffs);
        memcpy(tC, N, sizeof(float) * n_cm_coeffs);

        FirstRun = false;
    }
    else
    {
        for (int i = 0; i < n_cm_coeffs; i++)
        {
            tC[i] = (1.f - smooth) * tC[i] + smooth * N[i];
            dC[i] = (tC[i] - C[i]) * blockSizeInv;
        }
    }
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::ToNormalizedLattice(double a0inv, double a1, double a2,
                                                                 double b0, double b1, double b2,
                                                                 double g)
{
    b0 *= a0inv;
    b1 *= a0inv;
    b2 *= a0inv;
    a1 *= a0inv;
    a2 *= a0inv;

    double k1 = a1 / (1.0 + a2);
    double k2 = a2;

    double q1 = 1.0 - k1 * k1;
    double q2 = 1.0 - k2 * k2;
    q1 = sqrt(fabs(q1));
    q2 = sqrt(fabs(q2));

    double v3 = b2;
    double v2 = (b1 - a1 * v3) / q2;
    double v1 = (b0 - k1 * v2 * q2 - k2 * v3) / (q1 * q2);

    float N[n_cm_coeffs]{};
    memset(N, 0, sizeof(float) * n_cm_coeffs);
    N[0] = (float)k1;
    N[1] = (float)k2;
    N[2] = (float)q1;
    N[3] = (float)q2;
    N[4] = (float)v1;
    N[5] = (float)v2;
    N[6] = (float)v3;
    N[7] = (float)g;

    FromDirect(N);
}

template <typename TuningProvider>
void FilterCoefficientMaker<TuningProvider>::ToCoupledForm(double a0inv, double a1, double a2,
                                                           double b0, double b1, double b2,
                                                           double g)
{
    b0 *= a0inv;
    b1 *= a0inv;
    b2 *= a0inv;
    a1 *= a0inv;
    a2 *= a0inv;

    double ar, ai;
    double sq = a1 * a1 - 4.0 * a2;

    ar = 0.5 * -a1;
    sq = std::min(0.0, sq);
    ai = 0.5 * sqrt(-sq);
    ai = std::max(ai, 8.0 * 1.192092896e-07F);

    double bb1 = b1 - a1 * b0;
    double bb2 = b2 - a2 * b0;

    double d = b0;
    double c1 = bb1;
    double c2 = (bb1 * ar + bb2) / ai;

    float N[n_cm_coeffs]{};
    memset(N, 0, sizeof(float) * n_cm_coeffs);
    N[0] = (float)ar;
    N[1] = (float)ai;
    N[2] = 1.f;
    N[4] = (float)c1;
    N[5] = (float)c2;
    N[6] = (float)d;
    N[7] = (float)g;

    FromDirect(N);
}

template <typename TuningProvider> void FilterCoefficientMaker<TuningProvider>::Reset()
{
    FirstRun = true;
    memset(C, 0, sizeof(float) * n_cm_coeffs);
    memset(dC, 0, sizeof(float) * n_cm_coeffs);
    memset(tC, 0, sizeof(float) * n_cm_coeffs);

    provider = nullptr;
}

} // namespace sst::filters

#endif // SST_FILTERS_FILTERCOEFFICIENTMAKER_IMPL_H
