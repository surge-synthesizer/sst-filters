#ifndef _SST_CPP_FILTERS_H
#define _SST_CPP_FILTERS_H

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