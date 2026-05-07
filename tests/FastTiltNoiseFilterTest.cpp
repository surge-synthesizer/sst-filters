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

#include "catch2/catch2.hpp"
#include "sst/basic-blocks/simd/setup.h"
#include "sst/basic-blocks/dsp/FastMath.h"
#include "sst/filters/FastTiltNoiseFilter.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

namespace
{

static constexpr int GOLDEN_WARMUP = 100;
static constexpr int GOLDEN_RECORD = 50;
static constexpr float GOLDEN_SR = 48000.f;
static constexpr float GOLDEN_TOL = 1e-5f;

struct TestHost
{
    float srInv{1.f / GOLDEN_SR};
    float getSampleRateInv() const { return srInv; }
    float dbToLinear(float db) const { return std::pow(10.f, db / 20.f); }
};

static bool goldenPrintMode() { return std::getenv("SST_FILTERS_GOLDEN") != nullptr; }

static void goldenCheckOrPrint(const char *label, const std::array<float, GOLDEN_RECORD> &got,
                               const std::array<float, GOLDEN_RECORD> &expected)
{
    if (goldenPrintMode())
    {
        std::printf("// %s\n        {", label);
        for (int i = 0; i < GOLDEN_RECORD; ++i)
        {
            if (i > 0 && i % 5 == 0)
                std::printf("\n         ");
            std::printf("%.9ff%s", got[i], i + 1 < GOLDEN_RECORD ? ", " : "");
        }
        std::printf("};\n");
    }
    else
    {
        for (int i = 0; i < GOLDEN_RECORD; ++i)
        {
            INFO(label << " sample[" << i << "]: got=" << got[i] << " expected=" << expected[i]);
            REQUIRE(got[i] == Approx(expected[i]).margin(GOLDEN_TOL));
        }
    }
}

// Well-seeded uniform [-1, 1] noise via std::mt19937
struct NoiseSrc
{
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{-1.f, 1.f};
    NoiseSrc(uint32_t seed) : rng(seed) {}
    float next() { return dist(rng); }
};

} // namespace

TEST_CASE("FastTiltNoiseFilter golden — constant gain", "[FastTiltNoiseFilter][golden]")
{
    TestHost host;
    sst::filters::FastTiltNoiseFilter<TestHost> filt(host);

    NoiseSrc noise(0xC0FFEEu);

    // init wants 11 startup samples
    float startup[11];
    for (int i = 0; i < 11; ++i)
        startup[i] = noise.next();

    const float gainDb = 1.5f;
    filt.init(startup, gainDb);

    auto step = [&]() -> float {
        float v = noise.next();
        sst::filters::FastTiltNoiseFilter<TestHost>::step(filt, v);
        return v;
    };

    for (int i = 0; i < GOLDEN_WARMUP; ++i)
        step();

    std::array<float, GOLDEN_RECORD> got;
    for (int i = 0; i < GOLDEN_RECORD; ++i)
        got[i] = step();

    static const std::array<float, GOLDEN_RECORD> expected{
        0.466219068f,  -2.819243431f, 0.947164178f,  -2.445785522f, 0.522235870f,  0.092461437f,
        -2.655057907f, 3.792978764f,  -2.913748741f, 1.845941186f,  1.770492315f,  -1.199637651f,
        -3.001123905f, 3.269463301f,  1.488086581f,  1.250287533f,  -2.923462152f, 2.282492638f,
        -3.854727983f, 3.318052530f,  -0.856681705f, -0.372094870f, -1.646380544f, 1.560557723f,
        -2.846269846f, -0.587741256f, -1.135814667f, 0.301695734f,  2.313779831f,  -0.449219406f,
        -2.084875345f, 0.636168838f,  -0.955519974f, 3.797450066f,  -4.010819435f, -0.380552769f,
        2.770475388f,  0.105967462f,  2.446803093f,  -1.541170835f, -0.525261164f, 1.302263021f,
        2.182656527f,  -4.205843925f, 2.153871775f,  -1.229505062f, -2.057686567f, 3.297913074f,
        -3.630734444f, -0.533062935f};

    goldenCheckOrPrint("FastTiltNoiseFilter golden — constant gain", got, expected);
}

TEST_CASE("FastTiltNoiseFilter golden — modulated gain", "[FastTiltNoiseFilter][golden]")
{
    TestHost host;
    sst::filters::FastTiltNoiseFilter<TestHost> filt(host);

    NoiseSrc noise(0xBEEFCAFEu);

    float startup[11];
    for (int i = 0; i < 11; ++i)
        startup[i] = noise.next();

    filt.init(startup, 0.f);

    static constexpr int blockSize = 16;

    // Sweep gain across blocks to exercise setCoeffForBlock smoothing
    auto runBlock = [&](float gainDb) {
        filt.template setCoeffForBlock<blockSize>(gainDb);
        float in[blockSize], out[blockSize];
        for (int i = 0; i < blockSize; ++i)
            in[i] = noise.next();
        filt.template processBlock<blockSize>(in, out);
        return std::array<float, blockSize>{out[0],  out[1],  out[2],  out[3], out[4],  out[5],
                                            out[6],  out[7],  out[8],  out[9], out[10], out[11],
                                            out[12], out[13], out[14], out[15]};
    };

    // Warmup (at varying gains so the smoother is in a non-trivial state)
    for (int b = 0; b < GOLDEN_WARMUP / blockSize + 1; ++b)
    {
        float g = -6.f + 12.f * (b % 4) / 3.f;
        (void)runBlock(g);
    }

    // Record GOLDEN_RECORD samples while sweeping gain block-by-block
    std::array<float, GOLDEN_RECORD> got{};
    int idx = 0;
    int blk = 0;
    while (idx < GOLDEN_RECORD)
    {
        float g = -8.f + 16.f * ((blk++) % 5) / 4.f;
        auto out = runBlock(g);
        for (int i = 0; i < blockSize && idx < GOLDEN_RECORD; ++i)
            got[idx++] = out[i];
    }

    static const std::array<float, GOLDEN_RECORD> expected{
        0.332769036f,  2.518121243f,  1.233438492f,  -3.991149426f, 0.854468048f,  -0.421517551f,
        2.906049013f,  -0.163127989f, -0.745112479f, 0.003633145f,  0.463690162f,  -0.157314628f,
        -0.139557108f, -0.610062361f, -1.057113290f, -1.519798875f, -2.058714151f, -2.708582878f,
        -3.526903152f, -4.602595806f, -5.800243855f, -6.947521687f, -7.843013763f, -8.479543686f,
        -8.846570969f, -8.990482330f, -8.936952591f, -8.748849869f, -8.475415230f, -8.151423454f,
        -7.760484695f, -7.307942390f, -6.782300949f, -6.223828793f, -5.603557587f, -4.947343349f,
        -4.452180862f, -3.965977192f, -3.515913010f, -3.274574041f, -2.898166180f, -2.530298710f,
        -2.171301126f, -1.968484521f, -1.608959079f, -1.819925785f, -1.203920364f, -1.135356665f,
        -1.150898695f, -0.464004099f};

    goldenCheckOrPrint("FastTiltNoiseFilter golden — modulated gain", got, expected);
}
