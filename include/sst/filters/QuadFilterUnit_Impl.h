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
#include "QuadFilterUnit.h"
#include "FilterConfiguration.h"

#include "sst/basic-blocks/mechanics/simd-ops.h"
#include "sst/basic-blocks/dsp/Clippers.h"
#include "sst/utilities/SincTable.h"

#include "VintageLadders.h"
#include "OBXDFilter.h"
#include "K35Filter.h"
#include "DiodeLadder.h"
#include "CutoffWarp.h"
#include "ResonanceWarp.h"
#include "TriPoleFilter.h"
#include "CytomicSVFQuadForm.h"

namespace sst::filters
{

inline SIMD_M128 SVFLP12Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // F1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // Q1

    auto L = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[0], f->R[0]));
    auto H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[0]));
    auto B = SIMD_MM(add_ps)(f->R[0], SIMD_MM(mul_ps)(f->C[0], H));

    auto L2 = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    auto H2 = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L2), SIMD_MM(mul_ps)(f->C[1], B));
    auto B2 = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H2));

    f->R[0] = SIMD_MM(mul_ps)(B2, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(L2, f->R[2]);

    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[2], SIMD_MM(mul_ps)(B, B))));

    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Gain
    return SIMD_MM(mul_ps)(L2, f->C[3]);
}

inline SIMD_M128 SVFLP24Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // F1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // Q1

    auto L = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[0], f->R[0]));
    auto H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[0]));
    auto B = SIMD_MM(add_ps)(f->R[0], SIMD_MM(mul_ps)(f->C[0], H));

    L = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], B));
    B = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H));

    f->R[0] = SIMD_MM(mul_ps)(B, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(L, f->R[2]);

    in = L;

    L = SIMD_MM(add_ps)(f->R[4], SIMD_MM(mul_ps)(f->C[0], f->R[3]));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[3]));
    B = SIMD_MM(add_ps)(f->R[3], SIMD_MM(mul_ps)(f->C[0], H));

    L = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], B));
    B = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H));

    f->R[3] = SIMD_MM(mul_ps)(B, f->R[2]);
    f->R[4] = SIMD_MM(mul_ps)(L, f->R[2]);

    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[2], SIMD_MM(mul_ps)(B, B))));

    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Gain
    return SIMD_MM(mul_ps)(L, f->C[3]);
}

inline SIMD_M128 SVFHP24Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // F1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // Q1

    auto L = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[0], f->R[0]));
    auto H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[0]));
    auto B = SIMD_MM(add_ps)(f->R[0], SIMD_MM(mul_ps)(f->C[0], H));

    L = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], B));
    B = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H));

    f->R[0] = SIMD_MM(mul_ps)(B, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(L, f->R[2]);

    in = H;

    L = SIMD_MM(add_ps)(f->R[4], SIMD_MM(mul_ps)(f->C[0], f->R[3]));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[3]));
    B = SIMD_MM(add_ps)(f->R[3], SIMD_MM(mul_ps)(f->C[0], H));

    L = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], B));
    B = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H));

    f->R[3] = SIMD_MM(mul_ps)(B, f->R[2]);
    f->R[4] = SIMD_MM(mul_ps)(L, f->R[2]);

    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[2], SIMD_MM(mul_ps)(B, B))));

    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Gain
    return SIMD_MM(mul_ps)(H, f->C[3]);
}

inline SIMD_M128 SVFBP24Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // F1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // Q1

    auto L = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[0], f->R[0]));
    auto H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[0]));
    auto B = SIMD_MM(add_ps)(f->R[0], SIMD_MM(mul_ps)(f->C[0], H));

    L = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], B));
    B = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H));

    f->R[0] = SIMD_MM(mul_ps)(B, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(L, f->R[2]);

    in = B;

    L = SIMD_MM(add_ps)(f->R[4], SIMD_MM(mul_ps)(f->C[0], f->R[3]));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[3]));
    B = SIMD_MM(add_ps)(f->R[3], SIMD_MM(mul_ps)(f->C[0], H));

    L = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], B));
    B = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H));

    f->R[3] = SIMD_MM(mul_ps)(B, f->R[2]);
    f->R[4] = SIMD_MM(mul_ps)(L, f->R[2]);

    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[2], SIMD_MM(mul_ps)(B, B))));

    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Gain
    return SIMD_MM(mul_ps)(B, f->C[3]);
}

inline SIMD_M128 SVFHP12Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // F1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // Q1

    auto L = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[0], f->R[0]));
    auto H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[0]));
    auto B = SIMD_MM(add_ps)(f->R[0], SIMD_MM(mul_ps)(f->C[0], H));

    auto L2 = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    auto H2 = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L2), SIMD_MM(mul_ps)(f->C[1], B));
    auto B2 = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H2));

    f->R[0] = SIMD_MM(mul_ps)(B2, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(L2, f->R[2]);

    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[2], SIMD_MM(mul_ps)(B, B))));

    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Gain
    return SIMD_MM(mul_ps)(H2, f->C[3]);
}

inline SIMD_M128 SVFBP12Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // F1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // Q1

    auto L = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[0], f->R[0]));
    auto H = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L), SIMD_MM(mul_ps)(f->C[1], f->R[0]));
    auto B = SIMD_MM(add_ps)(f->R[0], SIMD_MM(mul_ps)(f->C[0], H));

    auto L2 = SIMD_MM(add_ps)(L, SIMD_MM(mul_ps)(f->C[0], B));
    auto H2 = SIMD_MM(sub_ps)(SIMD_MM(sub_ps)(in, L2), SIMD_MM(mul_ps)(f->C[1], B));
    auto B2 = SIMD_MM(add_ps)(B, SIMD_MM(mul_ps)(f->C[0], H2));

    f->R[0] = SIMD_MM(mul_ps)(B2, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(L2, f->R[2]);

    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[2], SIMD_MM(mul_ps)(B, B))));

    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Gain
    return SIMD_MM(mul_ps)(B2, f->C[3]);
}

inline SIMD_M128 IIR12Aquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // K2
    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Q2
    auto f2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[3], in),
                              SIMD_MM(mul_ps)(f->C[1], f->R[1])); // Q2*in - K2*R1
    auto g2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], in),
                              SIMD_MM(mul_ps)(f->C[3], f->R[1])); // K2*in + Q2*R1

    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // K1
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // Q1
    auto f1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[2], f2),
                              SIMD_MM(mul_ps)(f->C[0], f->R[0])); // Q1*f2 - K1*R0
    auto g1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[0], f2),
                              SIMD_MM(mul_ps)(f->C[2], f->R[0])); // K1*f2 + Q1*R0

    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // V1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // V2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // V3
    auto y =
        SIMD_MM(add_ps)(SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[6], g2), SIMD_MM(mul_ps)(f->C[5], g1)),
                        SIMD_MM(mul_ps)(f->C[4], f1));

    f->R[0] = f1;
    f->R[1] = g1;

    return y;
}

inline SIMD_M128 IIR12Bquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    auto f2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[3], in),
                              SIMD_MM(mul_ps)(f->C[1], f->R[1])); // Q2*in - K2*R1
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]);                 // K2
    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]);                 // Q2
    auto g2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], in),
                              SIMD_MM(mul_ps)(f->C[3], f->R[1])); // K2*in + Q2*R1

    auto f1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[2], f2),
                              SIMD_MM(mul_ps)(f->C[0], f->R[0])); // Q1*f2 - K1*R0
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]);                 // K1
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);                 // Q1
    auto g1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[0], f2),
                              SIMD_MM(mul_ps)(f->C[2], f->R[0])); // K1*f2 + Q1*R0

    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // V1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // V2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // V3
    auto y =
        SIMD_MM(add_ps)(SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[6], g2), SIMD_MM(mul_ps)(f->C[5], g1)),
                        SIMD_MM(mul_ps)(f->C[4], f1));

    f->R[0] = SIMD_MM(mul_ps)(f1, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(g1, f->R[2]);

    f->C[7] = SIMD_MM(add_ps)(f->C[7], f->dC[7]); // Clipgain
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);

    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[7], SIMD_MM(mul_ps)(y, y))));

    return y;
}

inline SIMD_M128 IIR12WDFquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // E1 * sc
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // E2 * sc
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // -E1 / sc
    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // -E2 / sc
    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // C1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // C2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // D

    auto y = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[0]), SIMD_MM(mul_ps)(f->C[6], in)),
        SIMD_MM(mul_ps)(f->C[5], f->R[1]));
    auto t = SIMD_MM(add_ps)(
        in, SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[2], f->R[0]), SIMD_MM(mul_ps)(f->C[3], f->R[1])));

    auto s1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(t, f->C[0]), f->R[0]);
    auto s2 = SIMD_MM(sub_ps)(SIMD_MM(setzero_ps)(),
                              SIMD_MM(add_ps)(SIMD_MM(mul_ps)(t, f->C[1]), f->R[1]));

    // f->R[0] = s1;
    // f->R[1] = s2;

    f->R[0] = SIMD_MM(mul_ps)(s1, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(s2, f->R[2]);

    f->C[7] = SIMD_MM(add_ps)(f->C[7], f->dC[7]); // Clipgain
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[7], SIMD_MM(mul_ps)(y, y))));

    return y;
}

inline SIMD_M128 IIR12CFCquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    // State-space with clipgain (2nd order, limit within register)

    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // ar
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // ai
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // b1
    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // c1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // c2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // d

    // y(i) = c1.*s(1) + c2.*s(2) + d.*x(i);
    // s1 = ar.*s(1) - ai.*s(2) + x(i);
    // s2 = ai.*s(1) + ar.*s(2);

    auto y = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[0]), SIMD_MM(mul_ps)(f->C[6], in)),
        SIMD_MM(mul_ps)(f->C[5], f->R[1]));
    auto s1 = SIMD_MM(add_ps)(
        SIMD_MM(mul_ps)(in, f->C[2]),
        SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[0], f->R[0]), SIMD_MM(mul_ps)(f->C[1], f->R[1])));
    auto s2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], f->R[0]), SIMD_MM(mul_ps)(f->C[0], f->R[1]));

    f->R[0] = SIMD_MM(mul_ps)(s1, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(s2, f->R[2]);

    f->C[7] = SIMD_MM(add_ps)(f->C[7], f->dC[7]); // Clipgain
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] =
        SIMD_MM(max_ps)(m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[7], SIMD_MM(mul_ps)(y, y))));

    return y;
}

inline SIMD_M128 IIR12CFLquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    // State-space with softer limiter

    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // (ar)
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // (ai)
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // b1
    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // c1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // c2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // d

    // y(i) = c1.*s(1) + c2.*s(2) + d.*x(i);
    // s1 = ar.*s(1) - ai.*s(2) + x(i);
    // s2 = ai.*s(1) + ar.*s(2);

    auto y = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[0]), SIMD_MM(mul_ps)(f->C[6], in)),
        SIMD_MM(mul_ps)(f->C[5], f->R[1]));
    auto ar = SIMD_MM(mul_ps)(f->C[0], f->R[2]);
    auto ai = SIMD_MM(mul_ps)(f->C[1], f->R[2]);
    auto s1 = SIMD_MM(add_ps)(
        SIMD_MM(mul_ps)(in, f->C[2]),
        SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(ar, f->R[0]), SIMD_MM(mul_ps)(ai, f->R[1])));
    auto s2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(ai, f->R[0]), SIMD_MM(mul_ps)(ar, f->R[1]));

    f->R[0] = s1;
    f->R[1] = s2;

    /*m = 1 ./ max(1,abs(y(i)));
    mr = mr.*0.99 + m.*0.01;*/

    // Limiter
    const auto m001 = SIMD_MM(set1_ps)(0.001f);
    const auto m099 = SIMD_MM(set1_ps)(0.999f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    const auto m2 = SIMD_MM(set1_ps)(2.0f);

    auto m = SIMD_MM(rsqrt_ps)(SIMD_MM(max_ps)(
        m1, SIMD_MM(mul_ps)(m2, SIMD_MM(and_ps)(y, basic_blocks::mechanics::m128_mask_absval))));
    f->R[2] = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->R[2], m099), SIMD_MM(mul_ps)(m, m001));

    return y;
}

inline SIMD_M128 IIR24CFCquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    // State-space with clipgain (2nd order, limit within register)

    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // ar
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // ai
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // b1

    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // c1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // c2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // d

    auto y = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[0]), SIMD_MM(mul_ps)(f->C[6], in)),
        SIMD_MM(mul_ps)(f->C[5], f->R[1]));
    auto s1 = SIMD_MM(add_ps)(
        SIMD_MM(mul_ps)(in, f->C[2]),
        SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[0], f->R[0]), SIMD_MM(mul_ps)(f->C[1], f->R[1])));
    auto s2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], f->R[0]), SIMD_MM(mul_ps)(f->C[0], f->R[1]));

    f->R[0] = SIMD_MM(mul_ps)(s1, f->R[2]);
    f->R[1] = SIMD_MM(mul_ps)(s2, f->R[2]);

    auto y2 = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[3]), SIMD_MM(mul_ps)(f->C[6], y)),
        SIMD_MM(mul_ps)(f->C[5], f->R[4]));
    auto s3 = SIMD_MM(add_ps)(
        SIMD_MM(mul_ps)(y, f->C[2]),
        SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[0], f->R[3]), SIMD_MM(mul_ps)(f->C[1], f->R[4])));
    auto s4 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], f->R[3]), SIMD_MM(mul_ps)(f->C[0], f->R[4]));

    f->R[3] = SIMD_MM(mul_ps)(s3, f->R[2]);
    f->R[4] = SIMD_MM(mul_ps)(s4, f->R[2]);

    f->C[7] = SIMD_MM(add_ps)(f->C[7], f->dC[7]); // Clipgain
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[2] = SIMD_MM(max_ps)(
        m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[7], SIMD_MM(mul_ps)(y2, y2))));

    return y2;
}

inline SIMD_M128 IIR24CFLquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    // State-space with softer limiter

    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // (ar)
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // (ai)
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // b1
    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // c1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // c2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // d

    auto ar = SIMD_MM(mul_ps)(f->C[0], f->R[2]);
    auto ai = SIMD_MM(mul_ps)(f->C[1], f->R[2]);

    auto y = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[0]), SIMD_MM(mul_ps)(f->C[6], in)),
        SIMD_MM(mul_ps)(f->C[5], f->R[1]));
    auto s1 = SIMD_MM(add_ps)(
        SIMD_MM(mul_ps)(in, f->C[2]),
        SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(ar, f->R[0]), SIMD_MM(mul_ps)(ai, f->R[1])));
    auto s2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(ai, f->R[0]), SIMD_MM(mul_ps)(ar, f->R[1]));

    f->R[0] = s1;
    f->R[1] = s2;

    auto y2 = SIMD_MM(add_ps)(
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[4], f->R[3]), SIMD_MM(mul_ps)(f->C[6], y)),
        SIMD_MM(mul_ps)(f->C[5], f->R[4]));
    auto s3 =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(y, f->C[2]), SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(ar, f->R[3]),
                                                                     SIMD_MM(mul_ps)(ai, f->R[4])));
    auto s4 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(ai, f->R[3]), SIMD_MM(mul_ps)(ar, f->R[4]));

    f->R[3] = s3;
    f->R[4] = s4;

    /*m = 1 ./ max(1,abs(y(i)));
    mr = mr.*0.99 + m.*0.01;*/

    // Limiter
    const auto m001 = SIMD_MM(set1_ps)(0.001f);
    const auto m099 = SIMD_MM(set1_ps)(0.999f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    const auto m2 = SIMD_MM(set1_ps)(2.0f);

    auto m = SIMD_MM(rsqrt_ps)(SIMD_MM(max_ps)(
        m1, SIMD_MM(mul_ps)(m2, SIMD_MM(and_ps)(y2, basic_blocks::mechanics::m128_mask_absval))));
    f->R[2] = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->R[2], m099), SIMD_MM(mul_ps)(m, m001));

    return y2;
}

inline SIMD_M128 IIR24Bquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]); // K2
    f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]); // Q2
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]); // K1
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]); // Q1
    f->C[4] = SIMD_MM(add_ps)(f->C[4], f->dC[4]); // V1
    f->C[5] = SIMD_MM(add_ps)(f->C[5], f->dC[5]); // V2
    f->C[6] = SIMD_MM(add_ps)(f->C[6], f->dC[6]); // V3

    auto f2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[3], in),
                              SIMD_MM(mul_ps)(f->C[1], f->R[1])); // Q2*in - K2*R1
    auto g2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], in),
                              SIMD_MM(mul_ps)(f->C[3], f->R[1])); // K2*in + Q2*R1
    auto f1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[2], f2),
                              SIMD_MM(mul_ps)(f->C[0], f->R[0])); // Q1*f2 - K1*R0
    auto g1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[0], f2),
                              SIMD_MM(mul_ps)(f->C[2], f->R[0])); // K1*f2 + Q1*R0
    f->R[0] = SIMD_MM(mul_ps)(f1, f->R[4]);
    f->R[1] = SIMD_MM(mul_ps)(g1, f->R[4]);
    auto y1 =
        SIMD_MM(add_ps)(SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[6], g2), SIMD_MM(mul_ps)(f->C[5], g1)),
                        SIMD_MM(mul_ps)(f->C[4], f1));

    f2 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[3], y1),
                         SIMD_MM(mul_ps)(f->C[1], f->R[3])); // Q2*in - K2*R1
    g2 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[1], y1),
                         SIMD_MM(mul_ps)(f->C[3], f->R[3])); // K2*in + Q2*R1
    f1 = SIMD_MM(sub_ps)(SIMD_MM(mul_ps)(f->C[2], f2),
                         SIMD_MM(mul_ps)(f->C[0], f->R[2])); // Q1*f2 - K1*R0
    g1 = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[0], f2),
                         SIMD_MM(mul_ps)(f->C[2], f->R[2])); // K1*f2 + Q1*R0
    f->R[2] = SIMD_MM(mul_ps)(f1, f->R[4]);
    f->R[3] = SIMD_MM(mul_ps)(g1, f->R[4]);
    auto y2 =
        SIMD_MM(add_ps)(SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[6], g2), SIMD_MM(mul_ps)(f->C[5], g1)),
                        SIMD_MM(mul_ps)(f->C[4], f1));

    f->C[7] = SIMD_MM(add_ps)(f->C[7], f->dC[7]); // Clipgain
    const auto m01 = SIMD_MM(set1_ps)(0.1f);
    const auto m1 = SIMD_MM(set1_ps)(1.0f);
    f->R[4] = SIMD_MM(max_ps)(
        m01, SIMD_MM(sub_ps)(m1, SIMD_MM(mul_ps)(f->C[7], SIMD_MM(mul_ps)(y2, y2))));

    return y2;
}

template <FilterSubType subtype>
inline SIMD_M128 LPMOOGquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]);
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]);
    f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);

    f->R[0] = basic_blocks::dsp::softclip8_ps(SIMD_MM(add_ps)(
        f->R[0], SIMD_MM(mul_ps)(
                     f->C[1], SIMD_MM(sub_ps)(
                                  SIMD_MM(sub_ps)(
                                      SIMD_MM(mul_ps)(in, f->C[0]),
                                      SIMD_MM(mul_ps)(f->C[2], SIMD_MM(add_ps)(f->R[3], f->R[4]))),
                                  f->R[0]))));
    f->R[1] = SIMD_MM(add_ps)(f->R[1], SIMD_MM(mul_ps)(f->C[1], SIMD_MM(sub_ps)(f->R[0], f->R[1])));
    f->R[2] = SIMD_MM(add_ps)(f->R[2], SIMD_MM(mul_ps)(f->C[1], SIMD_MM(sub_ps)(f->R[1], f->R[2])));
    f->R[4] = f->R[3];
    f->R[3] = SIMD_MM(add_ps)(f->R[3], SIMD_MM(mul_ps)(f->C[1], SIMD_MM(sub_ps)(f->R[2], f->R[3])));

    return f->R[subtype];
}

inline SIMD_M128 SNHquad(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]);
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]);

    f->R[0] = SIMD_MM(add_ps)(f->R[0], f->C[0]);

    auto mask = SIMD_MM(cmpgt_ps)(f->R[0], SIMD_MM(setzero_ps)());

    f->R[1] = SIMD_MM(or_ps)(SIMD_MM(andnot_ps)(mask, f->R[1]),
                             SIMD_MM(and_ps)(mask, basic_blocks::dsp::softclip_ps(SIMD_MM(sub_ps)(
                                                       in, SIMD_MM(mul_ps)(f->C[1], f->R[1])))));

    const auto m1 = SIMD_MM(set1_ps)(-1.f);
    f->R[0] = SIMD_MM(add_ps)(f->R[0], SIMD_MM(and_ps)(m1, mask));

    return f->R[1];
}

template <int COMB_SIZE, bool morph> // COMB_SIZE must be a power of 2
SIMD_M128 COMBquad_SSE2(QuadFilterUnitState *__restrict f, SIMD_M128 in)
{
    static_assert(utilities::SincTable::FIRipol_M ==
                  256); // changing the constant requires updating the code below
    const auto m256 = SIMD_MM(set1_ps)(256.f);
    const SIMD_M128I m0xff = SIMD_MM(set1_epi32)(0xff);

    f->C[0] = SIMD_MM(add_ps)(f->C[0], f->dC[0]);
    f->C[1] = SIMD_MM(add_ps)(f->C[1], f->dC[1]);

    if constexpr (morph)
    {
        f->C[2] = SIMD_MM(add_ps)(f->C[2], f->dC[2]);
        f->C[3] = SIMD_MM(add_ps)(f->C[3], f->dC[3]);
    }

    auto a = SIMD_MM(mul_ps)(f->C[0], m256);
    SIMD_M128I e = SIMD_MM(cvtps_epi32)(a);
    int DTi alignas(16)[4], SEi alignas(16)[4];
    SIMD_M128I DT = SIMD_MM(srli_epi32)(e, 8);
    SIMD_MM(store_si128)((SIMD_M128I *)DTi, DT);
    SIMD_M128I SE = SIMD_MM(and_si128)(e, m0xff);
    SE = SIMD_MM(sub_epi32)(m0xff, SE);
    SIMD_MM(store_si128)((SIMD_M128I *)SEi, SE);
    auto DBRead = SIMD_MM(setzero_ps)();

    for (int i = 0; i < 4; i++)
    {
        if (f->active[i])
        {
            int RP = (f->WP[i] - DTi[i] - utilities::SincTable::FIRoffset) & (COMB_SIZE - 1);

            // SINC interpolation (12 samples)
            auto a = SIMD_MM(loadu_ps)(&f->DB[i][RP]);
            SEi[i] *= (utilities::SincTable::FIRipol_N << 1);
            auto b = SIMD_MM(load_ps)(&utilities::globalSincTable.sinctable[SEi[i]]);
            auto o = SIMD_MM(mul_ps)(a, b);

            a = SIMD_MM(loadu_ps)(&f->DB[i][RP + 4]);
            b = SIMD_MM(load_ps)(&utilities::globalSincTable.sinctable[SEi[i] + 4]);
            o = SIMD_MM(add_ps)(o, SIMD_MM(mul_ps)(a, b));

            a = SIMD_MM(loadu_ps)(&f->DB[i][RP + 8]);
            b = SIMD_MM(load_ps)(&utilities::globalSincTable.sinctable[SEi[i] + 8]);
            o = SIMD_MM(add_ps)(o, SIMD_MM(mul_ps)(a, b));

            SIMD_MM(store_ss)((float *)&DBRead + i, sst::basic_blocks::mechanics::sum_ps_to_ss(o));
        }
    }

    auto d = SIMD_MM(add_ps)(in, SIMD_MM(mul_ps)(DBRead, f->C[1]));
    d = basic_blocks::dsp::softclip_ps(d);

    for (int i = 0; i < 4; i++)
    {
        if (f->active[i])
        {
            // Write to delaybuffer (with "anti-wrapping")
            auto t = SIMD_MM(load_ss)((float *)&d + i);
            SIMD_MM(store_ss)(&f->DB[i][f->WP[i]], t);
            if (f->WP[i] < utilities::SincTable::FIRipol_N)
                SIMD_MM(store_ss)(&f->DB[i][f->WP[i] + COMB_SIZE], t);

            // Increment write position
            f->WP[i] = (f->WP[i] + 1) & (COMB_SIZE - 1);
        }
    }
    return SIMD_MM(add_ps)(SIMD_MM(mul_ps)(f->C[3], DBRead), SIMD_MM(mul_ps)(f->C[2], in));
}

template <int32_t scaleTimes1000, SIMD_M128 (*F)(QuadFilterUnitState *__restrict, SIMD_M128)>
SIMD_M128 ScaleQFPtr(QuadFilterUnitState *__restrict s, SIMD_M128 in)
{
    const auto scale = SIMD_MM(set1_ps)(scaleTimes1000 / 1000.f);
    auto res = F(s, in);
    return SIMD_MM(mul_ps)(res, scale);
}

template <bool Compensated>
inline FilterUnitQFPtr GetCompensatedQFPtrFilterUnit(FilterType type, FilterSubType subtype)
{
    switch (type)
    {
    // basic filter types
    case fut_lp12:
    {
        switch (subtype)
        {
        case st_Standard:
            return SVFLP12Aquad;
        case st_Driven:
            return IIR12CFCquad;
        case st_Clean:
            return IIR12Bquad;
        default:
            break;
        }
    }
    case fut_lp24:
    {
        switch (subtype)
        {
        case st_Standard:
            return SVFLP24Aquad;
        case st_Driven:
            return IIR24CFCquad;
        case st_Clean:
            return IIR24Bquad;
        default:
            break;
        }
        break;
    }
    case fut_hp12:
    {
        switch (subtype)
        {
        case st_Standard:
            return SVFHP12Aquad;
        case st_Driven:
            return IIR12CFCquad;
        case st_Clean:
            return IIR12Bquad;
        default:
            break;
        }
        break;
    }
    case fut_hp24:
    {
        switch (subtype)
        {
        case st_Standard:
            return SVFHP24Aquad;
        case st_Driven:
            return IIR24CFCquad;
        case st_Clean:
            return IIR24Bquad;
        default:
            break;
        }
        break;
    }
    case fut_bp12:
    {
        switch (subtype)
        {
        case st_Standard:
            return SVFBP12Aquad;
        case st_Driven:
        case st_bp12_LegacyDriven:
            return IIR12CFCquad;
        case st_Clean:
        case st_bp12_LegacyClean:
            return IIR12Bquad;
        default:
            break;
        }
        break;
    }
    case fut_bp24:
    {
        switch (subtype)
        {
        case st_Standard:
            return SVFBP24Aquad;
        case st_Driven:
            return IIR24CFCquad;
        case st_Clean:
            return IIR24Bquad;
        default:
            break;
        }
        break;
    }
    case fut_notch12:
        return IIR12Bquad;
    case fut_notch24:
        return IIR24Bquad;
    case fut_apf:
        return IIR12Bquad;
    case fut_lpmoog:
        switch (subtype)
        {
        case st_lpmoog_6dB:
            return LPMOOGquad<st_lpmoog_6dB>;
        case st_lpmoog_12dB:
            return LPMOOGquad<st_lpmoog_12dB>;
        case st_lpmoog_18dB:
            return LPMOOGquad<st_lpmoog_18dB>;
        case st_lpmoog_24dB:
            return LPMOOGquad<st_lpmoog_24dB>;
        default:
            break;
        }
        break;
    case fut_SNH:
        return SNHquad;
    case fut_comb_pos:
    case fut_comb_neg:
        if (subtype & static_cast<int>(QFUSubtypeMasks::EXTENDED_COMB))
        {
            if (subtype == st_comb_continuous_neg || subtype == st_comb_continuous_pos ||
                subtype == st_comb_continuous_posneg)
                return COMBquad_SSE2<utilities::MAX_FB_COMB_EXTENDED, true>;
            else
                return COMBquad_SSE2<utilities::MAX_FB_COMB_EXTENDED, false>;
        }
        else
        {
            if (subtype == st_comb_continuous_neg || subtype == st_comb_continuous_pos ||
                subtype == st_comb_continuous_posneg)
                return COMBquad_SSE2<utilities::MAX_FB_COMB, true>;
            else
                return COMBquad_SSE2<utilities::MAX_FB_COMB, false>;
        }
    case fut_vintageladder:
        switch (subtype)
        {
        case st_vintage_type1:
        case st_vintage_type1_compensated:
            if constexpr (Compensated)
                // Scale up by 6dB = 1.994 amplitudes
                return ScaleQFPtr<1994, VintageLadder::RK::process>;
            else
                return VintageLadder::RK::process;
        case st_vintage_type2:
        case st_vintage_type2_compensated:
            return VintageLadder::Huov::process;
        case st_vintage_type3:
        case st_vintage_type3_compensated:
            return VintageLadder::Huov2010::process;
        default:
            break;
        }
        break;
    case fut_obxd_2pole_lp:
    case fut_obxd_2pole_hp:
    case fut_obxd_2pole_bp:
    case fut_obxd_2pole_n:
        // All the differences are in subtype wrangling in the coefficient maker
        return OBXDFilter::process_2_pole;
        break;
    case fut_obxd_4pole:
        switch (subtype)
        {
        case st_obxd4pole_6dB:
            return OBXDFilter::process_4_pole<OBXDFilter::LP6>;
        case st_obxd4pole_12dB:
            return OBXDFilter::process_4_pole<OBXDFilter::LP12>;
        case st_obxd4pole_18dB:
            return OBXDFilter::process_4_pole<OBXDFilter::LP18>;
        case st_obxd4pole_24dB:
            return OBXDFilter::process_4_pole<OBXDFilter::LP24>;
        case st_obxd4pole_broken24dB:
            return OBXDFilter::process_4_pole<OBXDFilter::LP24Broken>;
        case st_obxd4pole_morph:
            return OBXDFilter::process_4_pole<OBXDFilter::MORPH>;
        default:
            return nullptr;
        }

        break;

    case fut_obxd_xpander:
        switch (subtype)
        {
        case st_obxdxpander_lp1:
            return OBXDFilter::process_4_pole<OBXDFilter::LP6>;
        case st_obxdxpander_lp2:
            return OBXDFilter::process_4_pole<OBXDFilter::LP12>;
        case st_obxdxpander_lp3:
            return OBXDFilter::process_4_pole<OBXDFilter::LP18>;
        case st_obxdxpander_lp4:
            return OBXDFilter::process_4_pole<OBXDFilter::LP24>;
        case st_obxdxpander_hp1:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_HP1>;
        case st_obxdxpander_hp2:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_HP2>;
        case st_obxdxpander_hp3:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_HP3>;
        case st_obxdxpander_bp4:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_BP4>;
        case st_obxdxpander_bp2:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_BP2>;
        case st_obxdxpander_n2:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_N2>;
        case st_obxdxpander_ph3:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_PH3>;
        case st_obxdxpander_hp2lp1:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_HP2LP1>;
        case st_obxdxpander_hp3lp1:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_HP3LP1>;
        case st_obxdxpander_n2lp1:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_N2LP1>;
        case st_obxdxpander_ph3lp1:
            return OBXDFilter::process_4_pole<OBXDFilter::XPANDER_PH3LP1>;
        default:
            return nullptr;
        }
        break;

    case fut_k35_lp:
        if (Compensated && subtype == 2)
            return ScaleQFPtr<0700, K35Filter::process_lp>;
        else
            return K35Filter::process_lp;
        break;
    case fut_k35_hp:
        return K35Filter::process_hp;
        break;
    case fut_diode:
        switch (subtype)
        {
        case st_diode_6dB:
            return DiodeLadderFilter::process<st_diode_6dB>;
        case st_diode_12dB:
            return DiodeLadderFilter::process<st_diode_12dB>;
        case st_diode_18dB:
            return DiodeLadderFilter::process<st_diode_18dB>;
        case st_diode_24dB:
            return DiodeLadderFilter::process<st_diode_24dB>;
        default:
            break;
        }
        break;
    case fut_cutoffwarp_lp:
    case fut_cutoffwarp_hp:
    case fut_cutoffwarp_n:
    case fut_cutoffwarp_bp:
    case fut_cutoffwarp_ap:
        switch (subtype)
        {
        case st_cutoffwarp_tanh1:
            return CutoffWarp::process<st_cutoffwarp_tanh1>;
        case st_cutoffwarp_tanh2:
            return CutoffWarp::process<st_cutoffwarp_tanh2>;
        case st_cutoffwarp_tanh3:
            return CutoffWarp::process<st_cutoffwarp_tanh3>;
        case st_cutoffwarp_tanh4:
            return CutoffWarp::process<st_cutoffwarp_tanh4>;
        case st_cutoffwarp_softclip1:
            return CutoffWarp::process<st_cutoffwarp_softclip1>;
        case st_cutoffwarp_softclip2:
            return CutoffWarp::process<st_cutoffwarp_softclip2>;
        case st_cutoffwarp_softclip3:
            return CutoffWarp::process<st_cutoffwarp_softclip3>;
        case st_cutoffwarp_softclip4:
            return CutoffWarp::process<st_cutoffwarp_softclip4>;
        case st_cutoffwarp_ojd1:
            return CutoffWarp::process<st_cutoffwarp_ojd1>;
        case st_cutoffwarp_ojd2:
            return CutoffWarp::process<st_cutoffwarp_ojd2>;
        case st_cutoffwarp_ojd3:
            if constexpr (Compensated)
                return ScaleQFPtr<0400, CutoffWarp::process<st_cutoffwarp_ojd3>>;
            else
                return CutoffWarp::process<st_cutoffwarp_ojd3>;
        case st_cutoffwarp_ojd4:
            return CutoffWarp::process<st_cutoffwarp_ojd4>;
        default:
            break;
        }
        break;
    case fut_resonancewarp_lp:
    case fut_resonancewarp_hp:
    case fut_resonancewarp_n:
    case fut_resonancewarp_bp:
    case fut_resonancewarp_ap:
        switch (subtype)
        {
        case st_resonancewarp_tanh1:
            return ResonanceWarp::process<st_resonancewarp_tanh1>;
        case st_resonancewarp_tanh2:
            return ResonanceWarp::process<st_resonancewarp_tanh2>;
        case st_resonancewarp_tanh3:
            return ResonanceWarp::process<st_resonancewarp_tanh3>;
        case st_resonancewarp_tanh4:
            if constexpr (Compensated)
                return ScaleQFPtr<1584, ResonanceWarp::process<st_resonancewarp_tanh4>>;
            else
                return ResonanceWarp::process<st_resonancewarp_tanh4>;
        case st_resonancewarp_softclip1:
            return ResonanceWarp::process<st_resonancewarp_softclip1>;
        case st_resonancewarp_softclip2:
            return ResonanceWarp::process<st_resonancewarp_softclip2>;
        case st_resonancewarp_softclip3:
            return ResonanceWarp::process<st_resonancewarp_softclip3>;
        case st_resonancewarp_softclip4:
            return ResonanceWarp::process<st_resonancewarp_softclip4>;
        default:
            break;
        }
        break;
    case fut_tripole:
        switch (subtype)
        {
        case st_tripole_LLL1:
            return TriPoleFilter::process<st_tripole_LLL1>;
        case st_tripole_LHL1:
            return TriPoleFilter::process<st_tripole_LHL1>;
        case st_tripole_HLH1:
            return TriPoleFilter::process<st_tripole_HLH1>;
        case st_tripole_HHH1:
            return TriPoleFilter::process<st_tripole_HHH1>;
        case st_tripole_LLL2:
            return TriPoleFilter::process<st_tripole_LLL2>;
        case st_tripole_LHL2:
            return TriPoleFilter::process<st_tripole_LHL2>;
        case st_tripole_HLH2:
            return TriPoleFilter::process<st_tripole_HLH2>;
        case st_tripole_HHH2:
            return TriPoleFilter::process<st_tripole_HHH2>;
        case st_tripole_LLL3:
            return TriPoleFilter::process<st_tripole_LLL3>;
        case st_tripole_LHL3:
            return TriPoleFilter::process<st_tripole_LHL3>;
        case st_tripole_HLH3:
            return TriPoleFilter::process<st_tripole_HLH3>;
        case st_tripole_HHH3:
            return TriPoleFilter::process<st_tripole_HHH3>;
        default:
            break;
        }
        break;
    case fut_cytomic_svf:
        if (subtype == st_cytomic_lp)
            return cytomic_quadform::CytomicQuad<true>;
        else
            return cytomic_quadform::CytomicQuad;

    case fut_none:
    case num_filter_types:
        break;
    }

    // assert here
    return nullptr;
}

} // namespace sst::filters
