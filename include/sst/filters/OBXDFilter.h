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

#ifndef INCLUDE_SST_FILTERS_OBXDFILTER_H
#define INCLUDE_SST_FILTERS_OBXDFILTER_H

#include "QuadFilterUnit.h"
#include "FilterCoefficientMaker.h"

/**
 * This namespace contains an adaptation of the filter from
 * https://github.com/reales/OB-Xd/blob/master/Source/Engine/Filter.h
 */
namespace sst::filters::OBXDFilter
{
enum Poles
{
    TWO_POLE,
    FOUR_POLE
};

enum Obxd12dBCoeff
{
    g12,
    R12,
    multimode,
    bandpass,
    self_osc_push,
    n_obxd12_coeff
};

enum Obxd24dBCoeff
{
    g24,
    R24,
    rcor24,
    rcor24inv,
    pole_mix,         // mm
    pole_mix_inv_int, // mmch
    pole_mix_scaled,  // mmt
    n_obxd24_coeff
};

enum Params
{
    s1,
    s2,
    s3,
    s4,
};

static constexpr int ssew = 4;

const auto zero = SIMD_MM(set1_ps)(0.0f);
const auto nine_two_zero = SIMD_MM(set1_ps)(0.00920833f);
const auto zero_zero_five = SIMD_MM(set1_ps)(0.05f);
const auto eight_seven_six = SIMD_MM(set1_ps)(0.0876f);
const auto one_zero_three = SIMD_MM(set1_ps)(0.0103592f);
const auto one_eight_five = SIMD_MM(set1_ps)(0.185f);
const auto zero_four_five = SIMD_MM(set1_ps)(0.45f);
const auto zero_five = SIMD_MM(set1_ps)(0.5f);
const auto one = SIMD_MM(set1_ps)(1.0f);
const auto one_three_five = SIMD_MM(set1_ps)(1.035f);
const auto two = SIMD_MM(set1_ps)(2.0f);
const auto three = SIMD_MM(set1_ps)(3.0f);
const auto gainAdjustment2Pole = SIMD_MM(set1_ps)(0.74f);
const auto gainAdjustment4Pole = SIMD_MM(set1_ps)(0.6f);

template <typename TuningProvider>
inline void makeCoefficients(FilterCoefficientMaker<TuningProvider> *cm, Poles p, float freq,
                             float reso, int sub, float sampleRateInv, TuningProvider *provider,
                             bool continuousMorph = false, float morphPole = 0.f)
{
    float lC[n_cm_coeffs]{};
    float rcrate = sqrt(44000.0f * sampleRateInv);
    float cutoff =
        fmin(provider->note_to_pitch(freq + 69) * (float)TuningProvider::MIDI_0_FREQ, 22000.0f) *
        sampleRateInv * (float)M_PI;

    if (p == TWO_POLE)
    {
        lC[g12] = tanf(cutoff);
        lC[R12] = 1.0f - reso;
        lC[bandpass] = 0.f;

        switch (sub)
        {
        case 0: // lowpass
        case 4: // lowpass self-oscillation push
            lC[multimode] = 0.f;
            break;
        case 1: // bandpass
        case 5: // bandpass self-oscillation push
            lC[multimode] = 0.5;
            lC[bandpass] = 1.f;
            break;
        case 2: // highpass
        case 6: // highpass self-oscillation push
            lC[multimode] = 1.0;
            break;
        case 3: // notch
        case 7: // notch self-oscillation push
            lC[multimode] = 0.5;
            break;
        default:
            break;
        }

        lC[self_osc_push] = (sub > 3);
    }
    else
    {
        lC[rcor24] = (970.0f / 44000.0f) * rcrate;
        lC[rcor24inv] = 1.0f / lC[rcor24];
        lC[g24] = tanf(cutoff);
        lC[R24] = 3.5f * reso;
        if (continuousMorph)
        {
            auto fsub = morphPole;
            lC[pole_mix] = 1.f - (float)(fsub / 3.f);
            lC[pole_mix_inv_int] = (float)(int)(3.f - (float)fsub);
        }
        else
        {
            lC[pole_mix] = 1.f - ((float)sub / 3.f);
            lC[pole_mix_inv_int] = (float)(int)(3.f - (float)sub);
        }
        lC[pole_mix_scaled] = (lC[pole_mix] * 3) - lC[pole_mix_inv_int];
    }

    cm->FromDirect(lC);
}

inline SIMD_M128 diodePairResistanceApprox(SIMD_M128 x)
{
    // return (((((0.0103592f * x) + 0.00920833f) * x + 0.185f) * x + 0.05f) * x + 1.0f);
    return SIMD_MM(add_ps)(
        SIMD_MM(mul_ps)(
            SIMD_MM(add_ps)(
                SIMD_MM(mul_ps)(
                    SIMD_MM(add_ps)(
                        SIMD_MM(mul_ps)(
                            SIMD_MM(add_ps)(SIMD_MM(mul_ps)(one_zero_three, x), nine_two_zero), x),
                        one_eight_five),
                    x),
                zero_zero_five),
            x),
        one);
    // Taylor approximation of a slightly mismatched diode pair
}

// resolve 0-delay feedback
inline SIMD_M128 NewtonRaphson12dB(SIMD_M128 sample, QuadFilterUnitState *__restrict f)
{
    // calculating feedback non-linear transconducance and compensated for R (-1)
    // boosting non-linearity
    SIMD_M128 tCfb;
    auto selfOscEnabledMask = SIMD_MM(cmpeq_ps)(f->C[self_osc_push], one);
    auto selfOscOffVal =
        SIMD_MM(sub_ps)(diodePairResistanceApprox(SIMD_MM(mul_ps)(f->R[s1], eight_seven_six)), one);
    auto selfOscOnVal = SIMD_MM(sub_ps)(
        diodePairResistanceApprox(SIMD_MM(mul_ps)(f->R[s1], eight_seven_six)), one_three_five);
    tCfb = SIMD_MM(add_ps)(SIMD_MM(and_ps)(selfOscEnabledMask, selfOscOnVal),
                           SIMD_MM(andnot_ps)(selfOscEnabledMask, selfOscOffVal));

    // resolve linear feedback
    // float y = ((sample - 2*(s1*(R+tCfb)) - g*s1  - s2)/(1+ g*(2*(R+tCfb)+ g)));
    auto y = SIMD_MM(div_ps)(
        SIMD_MM(sub_ps)(
            SIMD_MM(sub_ps)(
                SIMD_MM(sub_ps)(
                    sample, SIMD_MM(mul_ps)(
                                two, SIMD_MM(mul_ps)(f->R[s1], SIMD_MM(add_ps)(f->C[R12], tCfb)))),
                SIMD_MM(mul_ps)(f->C[g12], f->R[s1])),
            f->R[s2]),
        SIMD_MM(add_ps)(
            one,
            SIMD_MM(mul_ps)(f->C[g12],
                            SIMD_MM(add_ps)(SIMD_MM(mul_ps)(two, SIMD_MM(add_ps)(f->C[R12], tCfb)),
                                            f->C[g12]))));

    return y;
}

inline SIMD_M128 process_2_pole(QuadFilterUnitState *__restrict f, SIMD_M128 sample)
{
    for (int i = 0; i < n_obxd12_coeff; i++)
    {
        f->C[i] = SIMD_MM(add_ps)(f->C[i], f->dC[i]);
    }

    // float v = ((sample- R * s1*2 - g2*s1 - s2)/(1+ R*g1*2 + g1*g2));
    auto v = NewtonRaphson12dB(sample, f);
    // float y1 = v * g + s1;
    auto y1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(v, f->C[g12]), f->R[s1]);
    // s1 = v * g + y1;
    f->R[s1] = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(v, f->C[g12]), y1);
    // float y2 = y1 * g + s2;
    auto y2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(y1, f->C[g12]), f->R[s2]);
    // s2 = y1 * g + y2;
    f->R[s2] = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(y1, f->C[g12]), y2);

    SIMD_M128 mc;
    auto mask_bp = SIMD_MM(cmpeq_ps)(f->C[bandpass], zero);
    auto bp_false = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(one, f->C[multimode]), y2),
                                    SIMD_MM(mul_ps)(f->C[multimode], v));
    auto mask = SIMD_MM(cmplt_ps)(f->C[multimode], zero_five);
    auto val1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(zero_five, f->C[multimode]), y2),
                                SIMD_MM(mul_ps)(f->C[multimode], y1));
    auto val2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(one, f->C[multimode]), y1),
                                SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(f->C[multimode], zero_five), v));
    auto bp_true = SIMD_MM(add_ps)(SIMD_MM(and_ps)(mask, val1), SIMD_MM(andnot_ps)(mask, val2));
    mc = SIMD_MM(add_ps)(SIMD_MM(and_ps)(mask_bp, bp_false), SIMD_MM(andnot_ps)(mask_bp, bp_true));
    return SIMD_MM(mul_ps)(mc, gainAdjustment2Pole);
}

inline SIMD_M128 NewtonRaphsonR24dB(SIMD_M128 sample, SIMD_M128 lpc,
                                    QuadFilterUnitState *__restrict f)
{
    // float ml = 1 / (1+g24);
    auto ml = SIMD_MM(div_ps)(one, SIMD_MM(add_ps)(one, f->C[g24]));
    // float S = (lpc * (lpc * (lpc * f->R[s1] + f->R[s2]) + f->R[s3]) + f->R[s4]) * ml;
    auto S = SIMD_MM(mul_ps)(
        SIMD_MM(add_ps)(
            SIMD_MM(mul_ps)(
                lpc,
                SIMD_MM(add_ps)(
                    SIMD_MM(mul_ps)(lpc, SIMD_MM(add_ps)(SIMD_MM(mul_ps)(lpc, f->R[s1]), f->R[s2])),
                    f->R[s3])),
            f->R[s4]),
        ml);
    // float G = lpc * lpc * lpc * lpc;
    auto G = SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(SIMD_MM(mul_ps)(lpc, lpc), lpc), lpc);
    // float y = (sample - f->C[R24] * S) / (1 + f->C[R24] * G);
    auto y = SIMD_MM(div_ps)(SIMD_MM(sub_ps)(sample, SIMD_MM(mul_ps)(f->C[R24], S)),
                             SIMD_MM(add_ps)(one, SIMD_MM(mul_ps)(f->C[R24], G)));

    return y;
}

inline static SIMD_M128 tptpc(SIMD_M128 &state, SIMD_M128 inp, SIMD_M128 cutoff)
{
    auto v = SIMD_MM(div_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(inp, state), cutoff),
                             SIMD_MM(add_ps)(one, cutoff));
    auto res = SIMD_MM(add_ps)(v, state);
    state = SIMD_MM(add_ps)(res, v);
    return res;
}

template <bool broken24db>
inline SIMD_M128 process_4_pole(QuadFilterUnitState *__restrict f, SIMD_M128 sample)
{
    for (int i = 0; i < n_obxd24_coeff; i++)
    {
        f->C[i] = SIMD_MM(add_ps)(f->C[i], f->dC[i]);
    }

    // float lpc = f->C[g] / (1 + f->C[g]);
    auto lpc = SIMD_MM(div_ps)(f->C[g24], SIMD_MM(add_ps)(one, f->C[g24]));

    // float y0 = NewtonRaphsonR24dB(sample,f->C[g],lpc);
    auto y0 = NewtonRaphsonR24dB(sample, lpc, f);

    // first lowpass in cascade
    // double v = (y0 - f->R[s1]) * lpc;
    auto v = SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(y0, f->R[s1]), lpc);

    // double res = v + f->R[s1];
    auto res = SIMD_MM(add_ps)(v, f->R[s1]);

    // f->R[s1] = res + v;
    f->R[s1] = SIMD_MM(add_ps)(res, v);

    // damping
    // f->R[s1] =atan(s1*rcor24)*rcor24inv;
    auto s1_rcor24 = SIMD_MM(mul_ps)(f->R[s1], f->C[rcor24]);

    // this array must be aligned to a 16-byte boundary for SSE store/load
    float s1_rcor24_arr alignas(16)[ssew];
    SIMD_MM(store_ps)(s1_rcor24_arr, s1_rcor24);

    for (int i = 0; i < ssew; i++)
    {
        if (f->active[i])
            s1_rcor24_arr[i] = atan(s1_rcor24_arr[i]);
        else
            s1_rcor24_arr[i] = 0.f;
    }

    s1_rcor24 = SIMD_MM(load_ps)(s1_rcor24_arr);
    f->R[s1] = SIMD_MM(mul_ps)(s1_rcor24, f->C[rcor24inv]);

    auto y1 = res;
    auto y2 = tptpc(f->R[s2], y1, f->C[g24]);
    auto y3 = tptpc(f->R[s3], y2, f->C[g24]);
    auto y4 = tptpc(f->R[s4], y3, f->C[g24]);

    SIMD_M128 mc;

    auto zero_mask = SIMD_MM(cmpeq_ps)(f->C[pole_mix_inv_int], zero);
    SIMD_M128 zero_val;
    if constexpr (broken24db)
    {
        // This second mul was mis-coded as an add.
        zero_val = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(one, f->C[pole_mix_scaled]), y4),
                                   SIMD_MM(add_ps)(f->C[pole_mix_scaled], y3));
    }
    else
    {
        zero_val = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(one, f->C[pole_mix_scaled]), y4),
                                   SIMD_MM(mul_ps)(f->C[pole_mix_scaled], y3));
    }

    auto one_mask = SIMD_MM(cmpeq_ps)(f->C[pole_mix_inv_int], one);
    auto one_val = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(one, f->C[pole_mix_scaled]), y3),
                                   SIMD_MM(mul_ps)(f->C[pole_mix_scaled], y2));

    auto two_mask = SIMD_MM(cmpeq_ps)(f->C[pole_mix_inv_int], two);
    auto two_val = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(SIMD_MM(sub_ps)(one, f->C[pole_mix_scaled]), y2),
                                   SIMD_MM(mul_ps)(f->C[pole_mix_scaled], y1));

    auto three_mask = SIMD_MM(cmpeq_ps)(f->C[pole_mix_inv_int], three);
    auto three_val = y1;
    mc = SIMD_MM(add_ps)(SIMD_MM(and_ps)(zero_mask, zero_val), SIMD_MM(and_ps)(one_mask, one_val));
    mc = SIMD_MM(add_ps)(mc, SIMD_MM(add_ps)(SIMD_MM(and_ps)(two_mask, two_val),
                                             SIMD_MM(and_ps)(three_mask, three_val)));

    // half volume compensation
    auto out =
        SIMD_MM(mul_ps)(mc, SIMD_MM(add_ps)(one, SIMD_MM(mul_ps)(f->C[R24], zero_four_five)));
    return SIMD_MM(mul_ps)(out, gainAdjustment4Pole);
}
} // namespace sst::filters::OBXDFilter

#endif // SST_FILTERS_OBXDFILTER_H
