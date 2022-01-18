#ifndef _SST_CPP_FILTERS_H
#define _SST_CPP_FILTERS_H

#include "sst/filters/QuadFilterUnit.h"

namespace sst
{
namespace filters
{
// typedef __m128 value_t;
typedef float value_t;

template <typename P> struct CoefficientMaker
{
    static float pitch(P *provider, int note) { return provider->note_to_pitch(note); }
};

struct FilterState
{
};
typedef value_t (*FilterEvaluator)(FilterState *__restrict, value_t in);

enum FilterTypes
{
    ft_off
};

FilterEvaluator *makeFilter(FilterTypes ft, int subtype) { return nullptr; }

} // namespace filters
} // namespace sst

#endif