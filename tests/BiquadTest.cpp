#include "sst/filters/BiquadFilter.h"

#include "TestUtils.h"

struct TP
{
    float noteToPitch(float f) { return 0; }
    float dbToLinear(float f) { return 1.f; }
    float getSampleRateInv() { return 1.0 / 48000; }
};

TEST_CASE("Simple Biquad")
{
    TP tp;
    sst::filters::Biquad::BiquadFilter<TP, 32> bq(&tp);

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
