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

#ifndef INCLUDE_SST_FILTERS_PLUS_PLUS_DETAILS_FILTER_IMPL_H
#define INCLUDE_SST_FILTERS_PLUS_PLUS_DETAILS_FILTER_IMPL_H

#include <cassert>

namespace sst::filtersplusplus
{
inline SIMD_M128 offFun(sst::filters::QuadFilterUnitState *__restrict, SIMD_M128 in) { return in; }

inline bool Filter::prepareInstance()
{
    reset();
    if (payload.filterModel == FilterModels::Off)
    {
        payload.func = offFun;
        payload.valid = true;
        return true;
    }

    auto [ft, st] = payload.resolveLegacyType();

    if (ft == sst::filters::FilterType::fut_none)
        return false;

    payload.func = GetQFPtrFilterUnit(ft, st);

    // technically we should also check active here
    assert((requiredDelayLinesSizes(getFilterModel(), getModelConfiguration()) == 0) ||
           payload.externalDelayLines[0] != nullptr);
    assert((requiredDelayLinesSizes(getFilterModel(), getModelConfiguration()) == 0) ||
           payload.externalDelayLines[1] != nullptr);
    assert((requiredDelayLinesSizes(getFilterModel(), getModelConfiguration()) == 0) ||
           payload.externalDelayLines[2] != nullptr);
    assert((requiredDelayLinesSizes(getFilterModel(), getModelConfiguration()) == 0) ||
           payload.externalDelayLines[3] != nullptr);

    return payload.func != nullptr;
}

inline int Filter::coefficientsExtraCount(FilterModels model, const ModelConfig &config)
{
    switch (model)
    {
    case FilterModels::OBXD_4Pole:
        return config.st == SlopeLevels::Slope_Morph ? 1 : 0;
    case FilterModels::Comb:
        return config.st == SlopeLevels::Comb_Bipolar_ContinuousMix ? 1 : 0;
    case FilterModels::CytomicSVF:
    {
        if (config.pt == PassTypes::Bell || config.pt == PassTypes::LowShelf ||
            config.pt == PassTypes::HighShelf)
        {
            return 1;
        }
        return 0;
    }
    default:
        return 0;
    }
    return 0;
}

inline void Filter::makeCoefficients(int voice, float cutoff, float resonance, float extra,
                                     float extra2, float extra3)
{
    assert(payload.valid);
    auto [type, subtype] = payload.currentLegacyType;
    // We may have in the future models which don't use the QFS so
    switch (payload.filterModel)
    {
    default:
    {
        // ToDo: Cache This
        payload.makers[voice].MakeCoeffs(cutoff, resonance, type, subtype, nullptr, false, extra,
                                         extra2, extra3);
    }
    break;
    }
}

inline void Filter::copyCoefficientsFromVoiceToVoice(int from, int to)
{
    for (int i = 0; i < sst::filters::n_cm_coeffs; ++i)
    {
        payload.makers[to].C[i] = payload.makers[from].C[i];
        payload.makers[to].tC[i] = payload.makers[from].dC[i];
        payload.makers[to].tC[i] = payload.makers[from].dC[i];
    }
}

inline void Filter::makeConstantCoefficients(int voice, float cutoff, float resonance, float extra,
                                             float extra2, float extra3)
{
    makeCoefficients(voice, cutoff, resonance, extra, extra2, extra3);
    for (int i = 0; i < sst::filters::n_cm_coeffs; ++i)
    {
        payload.makers[voice].C[i] = payload.makers[voice].tC[i];
        payload.makers[voice].dC[i] = 0;
    }
}
inline void Filter::prepareBlock()
{
    payload.qfuState.sampleRate = payload.sampleRate;
    payload.qfuState.sampleRateInv = payload.sampleRateInv;
    for (int i = 0; i < 4; ++i)
    {
        payload.qfuState.active[i] = payload.active[i];
        payload.qfuState.DB[i] = payload.externalDelayLines[i];

        if (payload.active[i])
        {
            payload.makers[i].updateState(payload.qfuState, i);
        }
    }
}

inline SIMD_M128 Filter::processSample(SIMD_M128 x)
{
    assert(payload.func);
    return payload.func(&payload.qfuState, x);
}

inline void Filter::concludeBlock()
{
    // bring the state back
    for (int i = 0; i < 4; ++i)
    {
        if (payload.active[i])
            payload.makers[i].updateCoefficients(payload.qfuState, i);
    }
}

inline std::vector<FilterModels> Filter::availableModels()
{
    return {FilterModels::VemberClassic, FilterModels::VemberAllPass, FilterModels::VemberLadder,
            FilterModels::OBXD_2Pole,    FilterModels::OBXD_4Pole,    FilterModels::K35,
            FilterModels::DiodeLadder,   FilterModels::VintageLadder, FilterModels::CutoffWarp,
            FilterModels::ResonanceWarp, FilterModels::CytomicSVF,    FilterModels::TriPole,
            FilterModels::Comb,          FilterModels::SampleAndHold};
}

inline size_t Filter::requiredDelayLinesSizes(FilterModels model, const ModelConfig &k)
{
    if (model == FilterModels::Comb)
        return sst::filters::utilities::MAX_FB_COMB + sst::filters::utilities::SincTable::FIRipol_N;
    return 0;
}

inline float Filter::processMonoSample(float in)
{
    auto res = processSample(SIMD_MM(set1_ps)(in));
    return SIMD_MM(cvtss_f32)(res);
}

inline void Filter::processStereoSample(float inL, float inR, float &outL, float &outR)
{
    auto res = processSample(SIMD_MM(set_ps)(0., 0., inR, inL));
    float rf alignas(16)[4];
    SIMD_MM(store_ps)(rf, res);
    outL = rf[0];
    outR = rf[1];
}

inline void Filter::processQuadSample(float in[4], float out[4])
{
    auto res = processSample(SIMD_MM(loadu_ps)(in));
    SIMD_MM(storeu_ps)(out, res);
}

} // namespace sst::filtersplusplus

#endif // FILTER_IMPL_H
