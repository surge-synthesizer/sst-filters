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

#ifndef INCLUDE_SST_FILTERS_CYTOMICSVFCASCADEQUADFORM_H
#define INCLUDE_SST_FILTERS_CYTOMICSVFCASCADEQUADFORM_H

#include "QuadFilterUnit.h"
#include "CytomicSVFCascade.h"

/*
 * The QuadFilterUnit form of CytomicSVFCascade. Subtypes are driven + 2 * compensated;
 * compensation is lowpass only.
 */

namespace sst::filters::cytomic_cascade_quadform
{
using Mode = cytomic_cascade::Mode;

template <typename TuningProvider>
void makeCoefficients(FilterCoefficientMaker<TuningProvider> *cm, float freq, float res,
                      int subtype, float sampleRateInv, TuningProvider *provider)
{
    float lC[n_cm_coeffs]{};
    auto hz = 440 * FilterCoefficientMaker<TuningProvider>::provider_note_to_pitch_ignoring_tuning(
                        provider, freq);
    if (subtype & 1)
        cytomic_cascade::detail::makeCoeffs<true>(hz, res, sampleRateInv, lC);
    else
        cytomic_cascade::detail::makeCoeffs<false>(hz, res, sampleRateInv, lC);
    cm->FromDirect(lC);
}

template <Mode M, bool Driven, bool Compensated>
inline SIMD_M128 process(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    cytomic_cascade::detail::advanceCoeffs(f->C, f->dC);
    return cytomic_cascade::detail::step<M, Driven, Compensated>(f->C, &f->R[0], &f->R[2], in);
}

template <Mode M> inline FilterUnitQFPtr processForSubtype(int subtype)
{
    switch (subtype)
    {
    case 0:
        return process<M, false, false>;
    case 1:
        return process<M, true, false>;
    case 2:
        if constexpr (M == Mode::Lowpass)
            return process<M, false, true>;
        break;
    case 3:
        if constexpr (M == Mode::Lowpass)
            return process<M, true, true>;
        break;
    }
    return nullptr;
}
} // namespace sst::filters::cytomic_cascade_quadform

#endif // INCLUDE_SST_FILTERS_CYTOMICSVFCASCADEQUADFORM_H
