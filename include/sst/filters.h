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
#ifndef INCLUDE_SST_FILTERS_H
#define INCLUDE_SST_FILTERS_H

static_assert(__cplusplus >= 202002L, "Surge team libraries have moved to C++ 20");

/** Parent namespace for all Surge Synth Team code. */
namespace sst
{

/** DSP code for Surge filters */
namespace filters
{

/**
 * Utility code needed for Surge filters.
 * Note that this code may be moved to a submodule
 * at some point in the future.
 */
namespace utilities
{
}

} // namespace filters

} // namespace sst

#include "sst/filters/VintageLadders.h"
#include "sst/filters/OBXDFilter.h"
#include "sst/filters/K35Filter.h"
#include "sst/filters/DiodeLadder.h"
#include "sst/filters/CutoffWarp.h"
#include "sst/filters/ResonanceWarp.h"
#include "sst/filters/TriPoleFilter.h"

#include "sst/filters/QuadFilterUnit_Impl.h"
#include "sst/filters/FilterCoefficientMaker_Impl.h"

#endif