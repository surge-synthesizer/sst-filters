/*
 * sst-filters - A header-only collection of SIMD filter
 * implementations by the Surge Synth Team
 *
 * Copyright 2019-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-filters is released under the Gnu General Public Licens
 * version 3 or later. Some of the filters in this package
 * originated in the version of Surge open sourced in 2018.
 *
 * All source in sst-filters available at
 * https://github.com/surge-synthesizer/sst-filters
 */
#ifndef INCLUDE_SST_UTILITIES_GLOBALS_H
#define INCLUDE_SST_UTILITIES_GLOBALS_H

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <cstring> // needed for memset/memcpy on GCC

#if MAC

#if defined(__x86_64__)
#else
#define ARM_NEON 1
#endif

#endif

#if LINUX
#if defined(__aarch64__) || defined(__arm__)
#define ARM_NEON 1
#endif
#endif

#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) ||                                   \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#else
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#endif

namespace sst::filters::utilities
{

#if MAC || LINUX
#include <strings.h>

static inline int _stricmp(const char *s1, const char *s2) { return strcasecmp(s1, s2); }
#endif

// In surge 1.3 we wanted the comb filters to ring a little longer, but
// we don't want to firce this on every client so make it ifdefable at
// build time
#ifndef SST_FILTERS_COMB_EXTENSION_FACTOR
#define SST_FILTERS_COMB_EXTENSION_FACTOR 2
#endif

constexpr int MAX_FB_COMB = 2048 * SST_FILTERS_COMB_EXTENSION_FACTOR; // must be 2^n
constexpr int MAX_FB_COMB_EXTENDED = 2048 * 64;
} // namespace sst::filters::utilities

#endif // SST_FILTERS_GLOBALS_H
