
/*
 * An important noce on Licensing. sst-filters is, by and large,
 * gpl3 code, but Andy kindly made his entire work on this filter
 * available to everyone, so this header file s available
 * for you to copy in an MIT licensed context. You will have to
 * implement your own fast tan sse or use tanh below, but basically
 * go anead and use it however you want, in the same spirit as andy's
 * sharing the work.
 */

#ifndef SSTFILTERS_CYTOMICSVF_H
#define SSTFILTERS_CYTOMICSVF_H

#include <algorithm>
#include <cmath>
#include <cassert>

#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) ||                                   \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#else
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#endif

#include "sst/basic-blocks/dsp/FastMath.h"

/*
 * An implementation of "Solving the continuous SVF equations using
 * trapezoidal integration and equivalent currents" by Andy Simpler
 * @ cytomic
 *
 * https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
 *
 * structured so you can do SSE-based parallel stereo and
 * with functions for each mode allowing minimal overhead
 *
 * Higher level functions like when to call the setCoeff
 * and how to run as a block are left as an exercise to the client of this
 * class. This is just the raw functions coded up.
 */

namespace sst::filters
{
struct CytomicSVF
{
    __m128 ic1eq{_mm_setzero_ps()}, ic2eq{_mm_setzero_ps()};
    __m128 g, k, gk, a1, a2, a3, m0, m1, m2;

    __m128 oneSSE{_mm_set1_ps(1.0)};
    __m128 negoneSSE{_mm_set1_ps(-1.0)};
    __m128 twoSSE{_mm_set1_ps(2.0)};
    __m128 negtwoSSE{_mm_set1_ps(-2.0)};
    enum Mode
    {
        LP,
        HP,
        BP,
        NOTCH,
        PEAK,
        ALL,

        // Note to use these three modes after you call
        // setCoef you need setCoefExtended with the mode
        BELL,
        LOW_SHELF,
        HIGH_SHELF
    };

    /*
     * Mode per above
     * freq is frequency in hz
     * Resonance is 0...1 resonance
     * srInv is the inverse sample rate
     * bellShelfAmp is only used in BELL/LOW_SHELF/HIGH_SHELF for the amplitude.
     * The cytomic documents use 10^db/40 scale but here we assume the pow happens
     * outside throught some other means
     */
    void setCoeff(Mode mode, float freq, float res, float srInv, float bellShelfAmp = 1.f)
    {
        auto conorm = std::clamp(freq * srInv, 0.f, 0.499f); // stable until nyquist
        res = std::clamp(res, 0.01f, 0.99f);
        bellShelfAmp = std::max(bellShelfAmp, 0.001f);

        g = _mm_set1_ps(sst::basic_blocks::dsp::fasttan(M_PI * conorm));
        k = _mm_div_ps(oneSSE, _mm_set1_ps(res));
        if (mode == BELL)
        {
            k = _mm_div_ps(k, _mm_set1_ps(bellShelfAmp));
        }
        gk = _mm_add_ps(g, k);
        a1 = _mm_div_ps(oneSSE, _mm_add_ps(oneSSE, _mm_mul_ps(g, gk)));
        a2 = _mm_mul_ps(g, a1);
        a3 = _mm_mul_ps(g, a2);

        switch (mode)
        {
        case LP:
            m0 = _mm_setzero_ps();
            m1 = _mm_setzero_ps();
            m2 = oneSSE;
            break;
        case BP:
            m0 = _mm_setzero_ps();
            m1 = oneSSE;
            m2 = _mm_setzero_ps();
            break;
        case HP:
            m0 = oneSSE;
            m1 = _mm_sub_ps(_mm_setzero_ps(), k);
            m2 = negoneSSE;
            break;
        case NOTCH:
            m0 = oneSSE;
            m1 = _mm_sub_ps(_mm_setzero_ps(), k);
            m2 = _mm_setzero_ps();
            break;
        case PEAK:
            m0 = oneSSE;
            m1 = _mm_sub_ps(_mm_setzero_ps(), k);
            m2 = negtwoSSE;
            break;
        case ALL:
            m0 = oneSSE;
            m1 = _mm_mul_ps(negtwoSSE, k);
            m2 = _mm_setzero_ps();
            break;
        case BELL:
        {
            auto A = _mm_set1_ps(bellShelfAmp);
            m0 = oneSSE;
            m1 = _mm_mul_ps(k, _mm_sub_ps(_mm_mul_ps(A, A), oneSSE));
            m2 = _mm_setzero_ps();
        }
        break;
        case LOW_SHELF:
        {
            auto A = _mm_set1_ps(bellShelfAmp);
            m0 = oneSSE;
            m1 = _mm_mul_ps(k, _mm_sub_ps(A, oneSSE));
            m2 = _mm_sub_ps(_mm_mul_ps(A, A), oneSSE);
        }
        break;
        case HIGH_SHELF:
        {
            auto A = _mm_set1_ps(bellShelfAmp);
            m0 = _mm_mul_ps(A, A);
            m1 = _mm_mul_ps(_mm_mul_ps(k, _mm_sub_ps(oneSSE, A)), A);
            m2 = _mm_sub_ps(oneSSE, _mm_mul_ps(A, A));
        }
        break;
        default:
            m0 = _mm_setzero_ps();
            m1 = _mm_setzero_ps();
            m2 = _mm_setzero_ps();
            break;
        }
    }

    static void step(CytomicSVF &that, float &L, float &R)
    {
        auto vin = _mm_set_ps(0, 0, R, L);
        auto res = stepSSE(that, vin);
        float r4 alignas(16)[4];
        _mm_store_ps(r4, res);
        L = r4[0];
        R = r4[1];
    }

    static __m128 stepSSE(CytomicSVF &that, __m128 vin)
    {
        // v3 = v0 - ic2eq
        auto v3 = _mm_sub_ps(vin, that.ic2eq);

        // v1 = a1 * ic1eq + a2 * v3
        auto v1 = _mm_add_ps(_mm_mul_ps(that.a1, that.ic1eq), _mm_mul_ps(that.a2, v3));

        // v2 = ic2eq + a2 * ic1eq + a3 * v3
        auto v2 = _mm_add_ps(that.ic2eq,
                             _mm_add_ps(_mm_mul_ps(that.a2, that.ic1eq), _mm_mul_ps(that.a3, v3)));

        // ic1eq = 2 * v1 - ic1eq
        that.ic1eq = _mm_sub_ps(_mm_mul_ps(that.twoSSE, v1), that.ic1eq);

        // ic2eq = 2 * v2 - ic2eq
        that.ic2eq = _mm_sub_ps(_mm_mul_ps(that.twoSSE, v2), that.ic2eq);

        return _mm_add_ps(_mm_mul_ps(that.m0, vin),
                          _mm_add_ps(_mm_mul_ps(that.m1, v1), _mm_mul_ps(that.m2, v2)));
    }

    void init()
    {
        ic1eq = _mm_setzero_ps();
        ic2eq = _mm_setzero_ps();
    }
};
} // namespace sst::filters

#endif // SHORTCIRCUITXT_CYTOMICSVF_H