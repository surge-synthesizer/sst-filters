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
    FOUR_POLE,
    XPANDER
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
    rcor24inv, // if this isn't last before mix, update 4 pole eval
    lastNonMorph = rcor24inv,
    y1mix, // These are only used in the morph case
    y2mix,
    y3mix,
    y4mix,
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
                             bool continuousMorph = false, float morphPole01 = 0.f)
{
    float lC[n_cm_coeffs]{};
    float rcrate = sqrt(44000.0f * sampleRateInv);
    float cutoff =
        fmin(FilterCoefficientMaker<TuningProvider>::provider_note_to_pitch(provider, freq + 69) *
                 (float)TuningProvider::MIDI_0_FREQ,
             22000.0f) *
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
    else if (p == FOUR_POLE || p == XPANDER)
    {
        lC[rcor24] = (970.0f / 44000.0f) * rcrate;
        lC[rcor24inv] = 1.0f / lC[rcor24];
        lC[g24] = tanf(cutoff);
        lC[R24] = 3.5f * reso;
        if (continuousMorph)
        {
            auto fsub = std::clamp(morphPole01, 0.f, 1.f) * 3.0;
            // OK so 3 -> all Y4, 2 -> all Y3, 1 all Y2, 1 all Y1
            if (fsub < 1)
            {
                lC[y4mix] = 0;
                lC[y3mix] = 0;
                lC[y2mix] = fsub;
                lC[y1mix] = 1 - fsub;
            }
            else if (fsub < 2)
            {
                auto lsub = fsub - 1;
                lC[y4mix] = 0;
                lC[y3mix] = lsub;
                lC[y2mix] = 1 - lsub;
                lC[y1mix] = 0;
            }
            else if (fsub < 3)
            {
                auto lsub = fsub - 2;
                lC[y4mix] = lsub;
                lC[y3mix] = 1 - lsub;
                lC[y2mix] = 0;
                lC[y1mix] = 0;
            }
            else
            {
                lC[y4mix] = 1;
                lC[y3mix] = 0;
                lC[y2mix] = 0;
                lC[y1mix] = 0;
            }
        }
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

inline SIMD_M128 NewtonRaphson24dB(SIMD_M128 sample, SIMD_M128 lpc,
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

enum FourPoleMode
{
    MORPH,
    LP6,
    LP12,
    LP18,
    LP24,
    LP24Broken,
    XPANDER_HP3,
    XPANDER_HP2,
    XPANDER_HP1,
    XPANDER_BP4,
    XPANDER_BP2,
    XPANDER_N2,
    XPANDER_PH3,
    XPANDER_HP2LP1,
    XPANDER_HP3LP1,
    XPANDER_N2LP1,
    XPANDER_PH3LP1,
};

template <FourPoleMode fpm>
inline SIMD_M128 process_4_pole(QuadFilterUnitState *__restrict f, SIMD_M128 sample)
{
    if constexpr (fpm == MORPH)
    {
        for (int i = 0; i < n_obxd24_coeff; i++)
        {
            f->C[i] = SIMD_MM(add_ps)(f->C[i], f->dC[i]);
        }
    }
    else
    {
        // don't need the pole mix evolved
        for (int i = 0; i <= lastNonMorph; i++)
        {
            f->C[i] = SIMD_MM(add_ps)(f->C[i], f->dC[i]);
        }
    }

    // float lpc = f->C[g] / (1 + f->C[g]);
    auto lpc = SIMD_MM(div_ps)(f->C[g24], SIMD_MM(add_ps)(one, f->C[g24]));

    // float y0 = NewtonRaphson24dB(sample,f->C[g],lpc);
    auto y0 = NewtonRaphson24dB(sample, lpc, f);

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

    // 6dB is 3, 12 2, 18 1, 24 zero
    if constexpr (fpm == FourPoleMode::LP6)
    {
        mc = y1;
    }
    else if constexpr (fpm == FourPoleMode::LP12)
    {
        mc = y2;
    }
    else if constexpr (fpm == FourPoleMode::LP18)
    {
        mc = y3;
    }
    else if constexpr (fpm == FourPoleMode::LP24)
    {
        mc = y4;
    }
    else if constexpr (fpm == FourPoleMode::LP24Broken)
    {
        mc = SIMD_MM(add_ps)(y3, y4);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_HP3)
    {
        // 1 -3 3 -1 0
        auto t1 = SIMD_MM(sub_ps)(y0, SIMD_MM(mul_ps)(y1, three));
        auto t2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(y2, three), y3);
        mc = SIMD_MM(add_ps)(t1, t2);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_HP2)
    {
        // 1 -2 1 0 0
        // or y0-y1 + y2-y1
        auto t1 = SIMD_MM(sub_ps)(y0, y1);
        auto t2 = SIMD_MM(sub_ps)(y2, y1);
        mc = SIMD_MM(add_ps)(t1, t2);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_HP1)
    {
        // 1 -1 0 0 0
        mc = SIMD_MM(sub_ps)(y0, y1);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_BP4)
    {
        //{0,  0,  2, -4,  2}, // BP4
        // 2y2 - 4y3 + 2y4
        // 2*((y2 - y3) + (y4 - y3))
        auto t1 = SIMD_MM(sub_ps)(y2, y3);
        auto t2 = SIMD_MM(sub_ps)(y4, y3);
        auto t3 = SIMD_MM(add_ps)(t1, t2);
        mc = SIMD_MM(mul_ps)(two, t3);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_BP2)
    {
        //{0, -2,  2,  0,  0}, // BP2
        auto t1 = SIMD_MM(sub_ps)(y2, y1);
        mc = SIMD_MM(mul_ps)(two, t1);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_N2)
    {
        //{1, -2,  2,  0,  0}, // N2
        // or 2 * (y2-y1) + y0
        auto t1 = SIMD_MM(sub_ps)(y2, y1);
        auto t2 = SIMD_MM(mul_ps)(two, t1);
        mc = SIMD_MM(add_ps)(y0, t2);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_PH3)
    {
        //{1, -3,  6, -4,  0}, // PH3
        // y0 - 3 y1 + 6 y2 - 4 y3
        auto t1 = SIMD_MM(sub_ps)(y0, SIMD_MM(mul_ps)(y1, three));
        auto t2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(y2, three), SIMD_MM(mul_ps)(y3, two));
        mc = SIMD_MM(add_ps)(t1, SIMD_MM(mul_ps)(two, t2));
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_HP2LP1)
    {
        //{0, -1,  2, -1,  0}, // HP2+LP1
        auto t1 = SIMD_MM(sub_ps)(y2, y1);
        auto t2 = SIMD_MM(sub_ps)(y2, y3);
        mc = SIMD_MM(add_ps)(t1, t2);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_HP3LP1)
    {
        //{0, -1,  3, -3,  1}, // HP3+LP1
        auto t1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(y2, three), y1);
        auto t2 = SIMD_MM(sub_ps)(y4, SIMD_MM(mul_ps)(y3, three));
        mc = SIMD_MM(add_ps)(t1, t2);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_N2LP1)
    {
        //{0, -1,  2, -2,  0}, // N2+LP1
        auto t1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(y2, two), y1);
        auto t2 = SIMD_MM(mul_ps)(y3, two);
        mc = SIMD_MM(sub_ps)(t1, t2);
    }
    else if constexpr (fpm == FourPoleMode::XPANDER_PH3LP1)
    {
        //{0, -1,  3, -6,  4}, // PH3+LP1
        auto t1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(y2, three), y1);
        auto t2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(y4, two), SIMD_MM(mul_ps)(y3, three));
        mc = SIMD_MM(add_ps)(t1, SIMD_MM(mul_ps)(two, t2));
    }
    else if constexpr (fpm == FourPoleMode::MORPH)
    {
        auto t1 = SIMD_MM(mul_ps)(f->C[y1mix], y1);
        auto t2 = SIMD_MM(mul_ps)(f->C[y2mix], y2);
        auto t3 = SIMD_MM(mul_ps)(f->C[y3mix], y3);
        auto t4 = SIMD_MM(mul_ps)(f->C[y4mix], y4);
        mc = SIMD_MM(add_ps)(t1, SIMD_MM(add_ps)(t2, SIMD_MM(add_ps)(t3, t4)));
    }

    // half volume compensation
    auto out =
        SIMD_MM(mul_ps)(mc, SIMD_MM(add_ps)(one, SIMD_MM(mul_ps)(f->C[R24], zero_four_five)));
    return SIMD_MM(mul_ps)(out, gainAdjustment4Pole);
}
} // namespace sst::filters::OBXDFilter

#endif // SST_FILTERS_OBXDFILTER_H
