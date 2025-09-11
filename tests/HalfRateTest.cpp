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
#include "sst/filters/HalfRateFilter.h"
#include "TestUtils.h"

template <int BS = 32>
std::array<double, 3> upDown(int M, bool steep, const std::function<float(float)> &gen)
{
    static constexpr int BSUP = BS << 1;
    static constexpr int nblocks = 256;
    float L alignas(16)[BSUP], R alignas(16)[BSUP];
    float Lu alignas(16)[BSUP], Ru alignas(16)[BSUP];
    float Ld alignas(16)[BSUP], Rd alignas(16)[BSUP];

    int smp = 0;
    int sr = 48000; /// this doesn't really matter except to create phase for the gen
    double sri = 1.0 / sr;

    double rmsUp{0.}, rmsDn{0.}, rmsSelf{0.};
    double avgRatioUp{0}, avgRatioDn{0};
    auto hrfUp = sst::filters::HalfRate::HalfRateFilter(M, steep);
    auto hrfDn = sst::filters::HalfRate::HalfRateFilter(M, steep);
    for (int i = 0; i < nblocks; ++i)
    {
        int s0 = i * BS;
        for (int s = 0; s < BS; ++s)
        {
            // This is just a test - doesn't have to be efficient
            float phase = (s0 + s) * sri;
            while (phase > 1)
                phase -= 1;

            L[s] = gen(phase);
            R[s] = gen(phase);
            smp++;
        }

        hrfUp.process_block_U2(L, R, Lu, Ru, BSUP);
        memcpy(Ld, Lu, BSUP * sizeof(float));
        memcpy(Rd, Ru, BSUP * sizeof(float));
        hrfDn.process_block_D2(Ld, Rd, BSUP);

        if (i > 1) // skip the first block to let it warm up
        {
            for (int s = 0; s < BSUP; ++s)
            {
                float phase = (s0 + s * 0.5) * sri;
                while (phase > 1)
                    phase -= 1;

                // For some reason, the filter attenuates by half vs the input signal?
                auto dL = Lu[s] - 0.5 * gen(phase);
                auto dR = Ru[s] - 0.5 * gen(phase);

                if (fabs(gen(phase)) > 0.0001)
                    avgRatioUp += Lu[s] / gen(phase);

                rmsUp += dL * dL + dR * dR;
            }

            for (int s = 0; s < BS; ++s)
            {
                float phase = (s0 + s) * sri;
                while (phase > 1)
                    phase -= 1;

                auto dL = Ld[s] - 0.5 * gen(phase);
                auto dR = Rd[s] - 0.5 * gen(phase);

                if (fabs(gen(phase)) > 0.0001)
                    avgRatioDn += Ld[s] / gen(phase);

                rmsDn += dL * dL + dR * dR;
            }

            for (int s = 0; s < BS; ++s)
            {
                auto dL = Ld[s] - Lu[s * 2];
                auto dR = Rd[s] - Ru[s * 2];

                rmsSelf += dL * dL + dR * dR;
            }
        }
    }

    rmsUp = sqrt(rmsUp);
    rmsDn = sqrt(rmsDn);
    rmsSelf = sqrt(rmsSelf);
    rmsUp /= (smp * 2);
    rmsDn /= smp;
    rmsSelf /= smp;

    // std::cout << "AvgRatio is " << avgRatioUp / ( smp * 2 ) << " " << avgRatioDn / (smp) <<
    // std::endl;
    return {rmsUp, rmsDn, rmsSelf};
};

TEST_CASE("Half Rate Filter")
{

    for (int order = 1; order <= sst::filters::HalfRate::halfrate_max_M; ++order)
    {
        for (const auto &steep : {true, false})
        {

            DYNAMIC_SECTION("Silence order=" << order << " steep=" << (steep ? "true" : "false"))
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep, [](float phase) { return 0.f; });

                REQUIRE(res[0] == 0.0);
                REQUIRE(res[1] == 0.0);
                REQUIRE(res[2] == 0.0);
            }

            DYNAMIC_SECTION("Naive Saw order=" << order << " steep=" << (steep ? "true" : "false"))
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep, [](float phase) { return 2 * phase - 1; });

                REQUIRE(res[0] < 5e-5);
                REQUIRE(res[1] < 5e-5);
                REQUIRE(res[2] < 5e-5);
            }

            DYNAMIC_SECTION("Slow Sin order=" << order << " steep=" << (steep ? "true" : "false"))
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res =
                    upDown(order, steep, [](float phase) { return std::sin(2 * M_PI * phase); });

                REQUIRE(res[0] < 5e-5);
                REQUIRE(res[1] < 5e-5);
                REQUIRE(res[2] < 5e-5);
                ;
            }

            DYNAMIC_SECTION("Fast Sin order=" << order << " steep=" << (steep ? "true" : "false"))
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep,
                                  [](float phase) { return std::sin(10 * 2 * M_PI * phase); });

                REQUIRE(res[0] < 5e-5);
                REQUIRE(res[1] < 5e-5);
                REQUIRE(res[2] < 5e-5);
            }
        }
    }
}

TEST_CASE("Half Rate a sample at a time")
{
    SECTION("Basic Works")
    {
        sst::filters::HalfRate::HalfRateFilter hrfU(6, true);
        sst::filters::HalfRate::HalfRateFilter hrfSU(6, true);

        sst::filters::HalfRate::HalfRateFilter hrfD(6, true);
        sst::filters::HalfRate::HalfRateFilter hrfSD(6, true);

        auto dph = 440.0 / 48000;

        static constexpr size_t blockSize{8};
        static constexpr size_t nPoints{256 * blockSize};
        float LupBW[nPoints << 1], LdnBW[nPoints], Lin[nPoints];
        float RupBW[nPoints << 1], RdnBW[nPoints], Rin[nPoints];
        for (int i = 0; i < nPoints; i += blockSize)
        {
            float Lloc[blockSize], Rloc[blockSize];
            for (int k = 0; k < blockSize; ++k)
            {
                auto idx = i + k;
                Lloc[k] = std::sin(dph * idx * 2.0 * M_PI);
                Rloc[k] = std::cos(2.0 * dph * idx * 2.0 * M_PI);
                Lin[idx] = Lloc[k];
                Rin[idx] = Rloc[k];
            }
            hrfU.process_block_U2_fullscale(Lloc, Rloc, &LupBW[2 * i], &RupBW[2 * i],
                                            2 * blockSize);
            hrfD.process_block_D2(&LupBW[2 * i], &RupBW[2 * i], 2 * blockSize, &LdnBW[i],
                                  &RdnBW[i]);
        }

        float Lup[2], Rup[2];

        for (int i = 0; i < nPoints; ++i)
        {
            float Lsm = std::sin(dph * i * 2.0 * M_PI);
            float Rsm = std::cos(2.0 * dph * i * 2.0 * M_PI);
            float Ldn{0.f}, Rdn{0.f};

            hrfSU.process_sample_U2(Lsm, Rsm, Lup, Rup);
            hrfSD.process_sample_D2(Lup, Rup, Ldn, Rdn);

            INFO("Testing at " << i);
            REQUIRE(Lsm == Lin[i]);
            REQUIRE(Rsm == Rin[i]);
            REQUIRE(Lup[0] == LupBW[i * 2]);
            REQUIRE(Lup[1] == LupBW[i * 2 + 1]);
            REQUIRE(Rup[0] == RupBW[i * 2]);
            REQUIRE(Rup[1] == RupBW[i * 2 + 1]);
            // Why a margni here? The shuffles to do it block wise slightly
            // re-order the extraction of the elements in the block version whic
            // gives a smidge of floating point noise
            REQUIRE(Ldn == Approx(LdnBW[i]).margin(1e-6));
            REQUIRE(Rdn == Approx(RdnBW[i]).margin(1e-6));
        }
    }
}
