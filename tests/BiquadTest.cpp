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
#include "sst/filters/BiquadFilter.h"

#include "TestUtils.h"

struct TP
{
};

TEST_CASE("Simple Biquad")
{
    using bqt = sst::filters::Biquad::BiquadFilter<TP, 32>;

    TP tp;
    bqt bq(&tp);

    bq.coeff_LP(0.6, 0.01);

    float x alignas(16)[32];
    float y alignas(16)[32];
    // a square wave through an LP will get attenuated
    float phase = 0;
    float dphase = 1.0 / 79.4;
    for (int i = 0; i < 50; ++i)
    {
        for (int j = 0; j < 32; ++j)
        {
            x[j] = phase > 0.5 ? 1 : -1;
            phase += dphase;
            if (phase > 0)
                phase -= 1;
        }
        bq.process_block_to(x, y);
        for (int j = 0; j < 32; ++j)
        {
            // bounded
            REQUIRE(abs(x[j]) >= abs(y[j]));
            // non-zero
            REQUIRE(abs(x[j] * y[j]) > 0);
        }
    }
}
