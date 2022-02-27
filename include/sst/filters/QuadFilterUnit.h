#ifndef _SST_CPP_QUADFILTERUNIT_H
#define _SST_CPP_QUADFILTERUNIT_H

#include "sst/utilities/globals.h"
#include "FilterCoefficientMaker.h"

namespace sst
{
namespace filters
{

constexpr int n_filter_registers = 16;

/*
 * These are defined in QuadFilterUnit_Impl.h
 */
struct alignas(16) QuadFilterUnitState
{
    __m128 C[n_cm_coeffs], dC[n_cm_coeffs]; // coefficients
    __m128 R[n_filter_registers];           // registers
    float *DB[4];                           // delay buffers
    int active[4]; // 0xffffffff if voice is active, 0 if not (usable as mask)
    int WP[4];     // comb write position

    // methods
    float sampleRate;
    float sampleRateInv;
};
typedef __m128 (*FilterUnitQFPtr)(QuadFilterUnitState *__restrict, __m128 in);
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
