#ifndef _SST_CPP_QUADFILTERUNIT_H
#define _SST_CPP_QUADFILTERUNIT_H

#include "sst/utilities/globals.h"
#include "FilterCoefficientMaker.h"

namespace sst
{
namespace filters
{
/** Max number of registers stored in each filter state. */
constexpr int n_filter_registers = 16;

/** State for a filter unit. */
struct alignas(16) QuadFilterUnitState
{
    /** Filter coefficients */
    __m128 C[n_cm_coeffs];

    /** Filter coefficients "delta" */
    __m128 dC[n_cm_coeffs];

    /** Filter state */
    __m128 R[n_filter_registers];

    /** Array of pointers to the filter's delay buffers */
    float *DB[4];

    /** 0xffffffff if voice is active, 0 if not (usable as mask) */
    int active[4];

    /** Write position for comb filters */
    int WP[4];

    /** Current sample rate */
    float sampleRate;

    /** Reciprocal of the sample rate */
    float sampleRateInv;
};

/** Typedef alias for a filter unit processing method. */
typedef __m128 (*FilterUnitQFPtr)(QuadFilterUnitState *__restrict, __m128 in);

/** Returns a filter unit pointer for a given filter type and sub-type. */
FilterUnitQFPtr GetQFPtrFilterUnit(FilterType type, FilterSubType subtype);

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

} // namespace filters
} // namespace sst

#endif // _SST_CPP_QUADFILTERUNIT_H
