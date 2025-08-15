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
#ifndef TESTS_TESTUTILS_H
#define TESTS_TESTUTILS_H

#include <array>
#include <iostream>
#include <numeric>
#include <vector>

#include <catch2/catch2.hpp>

#include <sst/filters.h>
#include <sst/filters++.h>

namespace TestUtils
{
using sst::filters::FilterType, sst::filters::FilterSubType;
namespace utilities = sst::filters::utilities;

constexpr bool printRMSs = false; // change this to true when generating test data

constexpr auto sampleRate = 48000.0f;
constexpr int blockSize = 2048;

constexpr int numTestFreqs = 5;
constexpr std::array<float, numTestFreqs> testFreqs{80.0f, 200.0f, 440.0f, 1000.0f, 10000.0f};

inline float runSine(sst::filters::QuadFilterUnitState &filterState,
                     sst::filters::FilterUnitQFPtr &filterUnitPtr, float testFreq, int numSamples)
{
    // reset filter state
    std::fill(filterState.R, &filterState.R[sst::filters::n_filter_registers],
              SIMD_MM(setzero_ps)());

    std::vector<float> y(numSamples, 0.0f);
    for (int i = 0; i < numSamples; ++i)
    {
        auto x = (float)std::sin(2.0 * M_PI * (double)i * testFreq / sampleRate);

        auto yVec = filterUnitPtr(&filterState, SIMD_MM(set_ps1)(x));

        float yArr alignas(16)[4];
        SIMD_MM(store_ps)(yArr, yVec);
        y[i] = yArr[0];
    }

    auto squareSum = std::inner_product(y.begin(), y.end(), y.begin(), 0.0f);
    auto rms = std::sqrt(squareSum / (float)numSamples);
    return 20.0f * std::log10(rms);
};

inline float runSine(sst::filtersplusplus::Filter &filter, float co, float re, float testFreq,
                     int numSamples)
{
    filter.reset();
    auto bs = filter.getBlockSize();

    std::vector<float> y(numSamples, 0.0f);
    for (int i = 0; i < numSamples; ++i)
    {
        if (i % bs == 0)
        {
            if (i != 0)
                filter.concludeBlock();
            filter.makeCoefficients(0, co, re);
            filter.prepareBlock();
        }
        auto x = (float)std::sin(2.0 * M_PI * (double)i * testFreq / sampleRate);

        auto yVec = filter.processSample(SIMD_MM(set1_ps)(x));

        float yArr alignas(16)[4];
        SIMD_MM(store_ps)(yArr, yVec);
        y[i] = yArr[0];
    }

    auto squareSum = std::inner_product(y.begin(), y.end(), y.begin(), 0.0f);
    auto rms = std::sqrt(squareSum / (float)numSamples);
    return 20.0f * std::log10(rms);
};

using RMSSet = std::array<float, numTestFreqs>;
struct TestConfig
{
    FilterType type;
    FilterSubType subType;
    RMSSet expRmsDBs;
    float cutoffFreq = 0.0f;
    float resonance = 0.5f;
};

static float delayBufferData[4][utilities::MAX_FB_COMB + utilities::SincTable::FIRipol_N]{};
static float delayBufferNew[4 * (utilities::MAX_FB_COMB + utilities::SincTable::FIRipol_N)]{};

template <typename TuningProvider>
inline void runTestProvider(const TestConfig &testConfig, TuningProvider *tp)
{
    auto filterState = sst::filters::QuadFilterUnitState{};
    for (int i = 0; i < 4; ++i)
    {
        std::fill(delayBufferData[i],
                  delayBufferData[i] + utilities::MAX_FB_COMB + utilities::SincTable::FIRipol_N,
                  0.0f);
        filterState.DB[i] = delayBufferData[i];
        filterState.active[i] = (int)0xffffffff;
        filterState.WP[i] = 0;
    }

    auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(testConfig.type, testConfig.subType);

    sst::filters::FilterCoefficientMaker<TuningProvider> coefMaker;
    coefMaker.setSampleRateAndBlockSize(sampleRate, blockSize);
    coefMaker.MakeCoeffs(testConfig.cutoffFreq, testConfig.resonance, testConfig.type,
                         testConfig.subType, tp, false);
    coefMaker.updateState(filterState);

    std::array<float, numTestFreqs> actualRMSs{};
    for (int i = 0; i < numTestFreqs; ++i)
    {
        auto rmsDB = runSine(filterState, filterUnitPtr, testFreqs[i], blockSize);

        if constexpr (!printRMSs)
            REQUIRE(rmsDB == Approx(testConfig.expRmsDBs[i]).margin(1.0e-2f));

        actualRMSs[i] = rmsDB;
    }

    if constexpr (printRMSs)
    {
        std::cout << "{ ";
        for (int i = 0; i < numTestFreqs; ++i)
            std::cout << actualRMSs[i] << "f, ";

        std::cout << "}" << std::endl;
    }
};

struct NonStaticTuningProvider
{
    NonStaticTuningProvider() = default;

    static constexpr double MIDI_0_FREQ = 8.17579891564371; // or 440.0 * pow( 2.0, - (69.0/12.0 ) )

    void note_to_omega_ignoring_tuning(float x, float &sinu, float &cosi, float sampleRate)
    {
        sst::filters::detail::BasicTuningProvider::note_to_omega_ignoring_tuning(x, sinu, cosi,
                                                                                 sampleRate);
    }

    float note_to_pitch_ignoring_tuning(float x)
    {
        return sst::filters::detail::BasicTuningProvider::note_to_pitch_ignoring_tuning(x);
    }

    float note_to_pitch_inv_ignoring_tuning(float x)
    {
        return sst::filters::detail::BasicTuningProvider::note_to_pitch_inv_ignoring_tuning(x);
    }

    float note_to_pitch(float x)
    {
        return sst::filters::detail::BasicTuningProvider::note_to_pitch(x);
    }
};

inline void runTest(const TestConfig &testConfig)
{
    runTestProvider<sst::filters::detail::BasicTuningProvider>(testConfig, nullptr);
    auto tp = std::make_unique<NonStaticTuningProvider>();
    runTestProvider<NonStaticTuningProvider>(testConfig, tp.get());
};

inline void runTest(sst::filtersplusplus::FilterModels model,
                    sst::filtersplusplus::ModelConfig config, float cutoff, float res,
                    std::array<float, numTestFreqs> answer)
{
    auto filter = sst::filtersplusplus::Filter();
    filter.setFilterModel(model);
    filter.setModelConfiguration(config);
    filter.setSampleRateAndBlockSize(sampleRate, blockSize);

    memset(delayBufferNew, 0, sizeof(delayBufferNew));
    for (int i = 0; i < 4; ++i)
    {
        filter.setActive(i, true);
        if (auto rs = filter.requiredDelayLinesSizes(model, config))
        {
            filter.provideDelayLine(i, &delayBufferNew[0] + rs * i);
        }
    }
    REQUIRE(filter.prepareInstance());

    std::array<float, numTestFreqs> actualRMSs{};
    for (int i = 0; i < numTestFreqs; ++i)
    {
        auto rmsDB = runSine(filter, cutoff, res, testFreqs[i], blockSize);

        if constexpr (!printRMSs)
        {
            INFO("Comparing at point " << i);
            REQUIRE(rmsDB == Approx(answer[i]).margin(1.0e-2f));
        }
        actualRMSs[i] = rmsDB;
    }

    if constexpr (printRMSs)
    {
        std::cout << "{ ";
        for (int i = 0; i < numTestFreqs; ++i)
            std::cout << actualRMSs[i] << "f, ";

        std::cout << "}" << std::endl;
    }
};

} // namespace TestUtils

#endif // TESTS_TESTUTILS_H
