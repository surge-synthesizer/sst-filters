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
#ifndef INCLUDE_SST_UTILITIES_SHARED_H
#define INCLUDE_SST_UTILITIES_SHARED_H

#include "globals.h"

namespace sst::filters::utilities
{

inline float i2f_binary_cast(int i)
{
    float *f = (float *)&i;
    return *f;
}

const auto m128_mask_signbit = SIMD_MM(set1_ps)(i2f_binary_cast(0x80000000));
const auto m128_mask_absval = SIMD_MM(set1_ps)(i2f_binary_cast(0x7fffffff));
const auto m128_zero = SIMD_MM(set1_ps)(0.0f);
const auto m128_half = SIMD_MM(set1_ps)(0.5f);
const auto m128_one = SIMD_MM(set1_ps)(1.0f);
const auto m128_two = SIMD_MM(set1_ps)(2.0f);
const auto m128_four = SIMD_MM(set1_ps)(4.0f);
const auto m128_1234 = SIMD_MM(set_ps)(1.f, 2.f, 3.f, 4.f);
const auto m128_0123 = SIMD_MM(set_ps)(0.f, 1.f, 2.f, 3.f);

} // namespace sst::filters::utilities

#endif // SST_FILTERS_SHARED_H
