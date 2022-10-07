#include "sst/filters/HalfRateFilter.h"

#include "TestUtils.h"

template<int BS=32>
std::array<double, 3> upDown(int M, bool steep, const std::function<float(float)> &gen)
{
    static constexpr int BSUP=BS << 1;
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
    for (int i=0; i<nblocks; ++i)
    {
        int s0 = i * BS;
        for (int s=0; s<BS; ++s)
        {
            // This is just a test - doesn't have to be efficient
            float phase = (s0 + s) * sri;
            while(phase > 1) phase -= 1;

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

            for (int s=0; s<BS; ++s)
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

    // std::cout << "AvgRatio is " << avgRatioUp / ( smp * 2 ) << " " << avgRatioDn / (smp) << std::endl;
    return {rmsUp, rmsDn, rmsSelf};
};

TEST_CASE("HalfRate Filter") {

    for (int order=1; order <=sst::filters::HalfRate::halfrate_max_M; ++order)
    {
        for (const auto &steep : { true, false})
        {

            DYNAMIC_SECTION( "Silence order=" << order << " steep=" << (steep? "true":"false") )
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep, [](float phase) { return 0.f; });

                REQUIRE(res[0] == 0.0);
                REQUIRE(res[1] == 0.0);
                REQUIRE(res[2] == 0.0);
            }

            DYNAMIC_SECTION( "Naive Saw order=" << order << " steep=" << (steep? "true":"false") )
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep, [](float phase) { return 2 * phase - 1; });

                REQUIRE(res[0] < 5e-5);
                REQUIRE(res[1] < 5e-5);
                REQUIRE(res[2] < 5e-5);
            }

            DYNAMIC_SECTION( "Slow Sin order=" << order << " steep=" << (steep? "true":"false") )
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep, [](float phase) { return std::sin(2 * M_PI * phase); });

                REQUIRE(res[0] < 5e-5);
                REQUIRE(res[1] < 5e-5);
                REQUIRE(res[2] < 5e-5);;
            }


            DYNAMIC_SECTION( "Fast Sin order=" << order << " steep=" << (steep? "true":"false") )
            {
                auto hrf = sst::filters::HalfRate::HalfRateFilter(order, steep);
                auto res = upDown(order, steep, [](float phase) { return std::sin(10 * 2 * M_PI * phase); });

                REQUIRE(res[0] < 5e-5);
                REQUIRE(res[1] < 5e-5);
                REQUIRE(res[2] < 5e-5);
            }
        }
    }
}
