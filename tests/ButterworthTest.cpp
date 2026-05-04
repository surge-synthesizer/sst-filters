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
#include "sst/filters/ButterworthLPHP.h"

#include <cmath>
#include <numbers>
#include <vector>

namespace
{

// Cascaded TPT Butterworth after bilinear transform has the closed form
//   |H_LP(f)|^2 = 1 / (1 + (Wf/Wc)^(2N))
//   |H_HP(f)|^2 = 1 / (1 + (Wc/Wf)^(2N))
// where W = tan(pi * freq / sr) is the prewarped frequency.
double expectedLPDb(double freq, double cutoff, double sr, int N)
{
    const double Wf = std::tan(std::numbers::pi * freq / sr);
    const double Wc = std::tan(std::numbers::pi * cutoff / sr);
    const double r = Wf / Wc;
    const double mag2 = 1.0 / (1.0 + std::pow(r, 2.0 * N));
    return 10.0 * std::log10(mag2);
}

double expectedHPDb(double freq, double cutoff, double sr, int N)
{
    const double Wf = std::tan(std::numbers::pi * freq / sr);
    const double Wc = std::tan(std::numbers::pi * cutoff / sr);
    const double r = Wc / Wf;
    const double mag2 = 1.0 / (1.0 + std::pow(r, 2.0 * N));
    return 10.0 * std::log10(mag2);
}

// Drive a sine into the filter, skip the initial transient, return RMS in dB.
template <typename Filter> double measureMonoDb(Filter &f, double freq, double sr)
{
    f.reset();
    const int total = static_cast<int>(sr * 0.5); // half a second
    const int skip = total / 4;
    double acc = 0.0;
    int count = 0;
    for (int n = 0; n < total; ++n)
    {
        const float x = static_cast<float>(std::sin(2.0 * std::numbers::pi * freq * n / sr));
        const float y = f.processSample(x);
        if (n >= skip)
        {
            acc += static_cast<double>(y) * y;
            ++count;
        }
    }
    // Reference RMS of unit-amplitude sine is 1/sqrt(2).
    const double rms = std::sqrt(acc / count);
    return 20.0 * std::log10(rms / (1.0 / std::sqrt(2.0)));
}

template <typename Filter>
std::pair<double, double> measureStereoDb(Filter &f, double freqL, double freqR, double sr)
{
    f.reset();
    const int total = static_cast<int>(sr * 0.5);
    const int skip = total / 4;
    double accL = 0.0, accR = 0.0;
    int count = 0;
    for (int n = 0; n < total; ++n)
    {
        float l = static_cast<float>(std::sin(2.0 * std::numbers::pi * freqL * n / sr));
        float r = static_cast<float>(std::sin(2.0 * std::numbers::pi * freqR * n / sr));
        f.processSample(l, r);
        if (n >= skip)
        {
            accL += static_cast<double>(l) * l;
            accR += static_cast<double>(r) * r;
            ++count;
        }
    }
    const double rmsL = std::sqrt(accL / count);
    const double rmsR = std::sqrt(accR / count);
    const double ref = 1.0 / std::sqrt(2.0);
    return {20.0 * std::log10(rmsL / ref), 20.0 * std::log10(rmsR / ref)};
}

// Float arithmetic in the cascade hits a noise floor near -100 dB; below that,
// just verify measured response is at least that deep.
constexpr double kNoiseFloorDb = -100.0;

inline void compareDb(double measured, double expected, double margin)
{
    if (expected < kNoiseFloorDb)
    {
        REQUIRE(measured < kNoiseFloorDb);
    }
    else
    {
        REQUIRE(measured == Approx(expected).margin(margin));
    }
}

template <int N>
void runLPSweep(double sr, double cutoff, const std::vector<double> &freqs, double margin = 0.5)
{
    sst::filters::ButterworthLP<N> f;
    f.setCutoffAndSampleRate(static_cast<float>(cutoff), static_cast<float>(sr));
    for (auto freq : freqs)
    {
        const double measured = measureMonoDb(f, freq, sr);
        const double expected = expectedLPDb(freq, cutoff, sr, N);
        INFO("LP N=" << N << " sr=" << sr << " fc=" << cutoff << " f=" << freq
                     << " measured=" << measured << " expected=" << expected);
        compareDb(measured, expected, margin);
    }
}

template <int N>
void runHPSweep(double sr, double cutoff, const std::vector<double> &freqs, double margin = 0.5)
{
    sst::filters::ButterworthHP<N> f;
    f.setCutoffAndSampleRate(static_cast<float>(cutoff), static_cast<float>(sr));
    for (auto freq : freqs)
    {
        const double measured = measureMonoDb(f, freq, sr);
        const double expected = expectedHPDb(freq, cutoff, sr, N);
        INFO("HP N=" << N << " sr=" << sr << " fc=" << cutoff << " f=" << freq
                     << " measured=" << measured << " expected=" << expected);
        compareDb(measured, expected, margin);
    }
}

} // namespace

TEST_CASE("Butterworth LP passband and rolloff at 96kHz, fc=5kHz")
{
    const double sr = 96000.0;
    const double fc = 5000.0;

    SECTION("Passband: 1k and 2k essentially unattenuated")
    {
        // Deep passband: > 1 octave below fc. N=4 and N=6 are nearly flat;
        // N=2 has a small but non-zero rolloff to which we compare exactly.
        runLPSweep<2>(sr, fc, {1000.0, 2000.0}, 0.3);
        runLPSweep<4>(sr, fc, {1000.0, 2000.0}, 0.3);
        runLPSweep<6>(sr, fc, {1000.0, 2000.0}, 0.3);
    }

    SECTION("At cutoff: -3 dB")
    {
        sst::filters::ButterworthLP<2> f2;
        sst::filters::ButterworthLP<4> f4;
        sst::filters::ButterworthLP<6> f6;
        f2.setCutoffAndSampleRate(fc, sr);
        f4.setCutoffAndSampleRate(fc, sr);
        f6.setCutoffAndSampleRate(fc, sr);
        REQUIRE(measureMonoDb(f2, fc, sr) == Approx(-3.0103).margin(0.2));
        REQUIRE(measureMonoDb(f4, fc, sr) == Approx(-3.0103).margin(0.2));
        REQUIRE(measureMonoDb(f6, fc, sr) == Approx(-3.0103).margin(0.2));
    }

    SECTION("Stopband rolloff at 5k, 10k, 20k for N=2,4,6")
    {
        const std::vector<double> stopband{5000.0, 10000.0, 20000.0};
        runLPSweep<2>(sr, fc, stopband);
        runLPSweep<4>(sr, fc, stopband);
        runLPSweep<6>(sr, fc, stopband);
    }
}

TEST_CASE("Butterworth HP passband and rolloff at 96kHz, fc=5kHz")
{
    const double sr = 96000.0;
    const double fc = 5000.0;

    SECTION("Passband: 10k and 20k essentially unattenuated")
    {
        runHPSweep<2>(sr, fc, {10000.0, 20000.0}, 0.3);
        runHPSweep<4>(sr, fc, {10000.0, 20000.0}, 0.3);
        runHPSweep<6>(sr, fc, {10000.0, 20000.0}, 0.3);
    }

    SECTION("At cutoff: -3 dB")
    {
        sst::filters::ButterworthHP<2> f2;
        sst::filters::ButterworthHP<4> f4;
        sst::filters::ButterworthHP<6> f6;
        f2.setCutoffAndSampleRate(fc, sr);
        f4.setCutoffAndSampleRate(fc, sr);
        f6.setCutoffAndSampleRate(fc, sr);
        REQUIRE(measureMonoDb(f2, fc, sr) == Approx(-3.0103).margin(0.2));
        REQUIRE(measureMonoDb(f4, fc, sr) == Approx(-3.0103).margin(0.2));
        REQUIRE(measureMonoDb(f6, fc, sr) == Approx(-3.0103).margin(0.2));
    }

    SECTION("Stopband rolloff at 5k, 2k, 1k for N=2,4,6")
    {
        const std::vector<double> stopband{5000.0, 2000.0, 1000.0};
        runHPSweep<2>(sr, fc, stopband);
        runHPSweep<4>(sr, fc, stopband);
        runHPSweep<6>(sr, fc, stopband);
    }
}

TEST_CASE("Butterworth at 88200 Hz with various cutoffs")
{
    const double sr = 88200.0;

    SECTION("LP fc=2kHz")
    {
        runLPSweep<2>(sr, 2000.0, {200.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0});
        runLPSweep<4>(sr, 2000.0, {200.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0});
        runLPSweep<6>(sr, 2000.0, {200.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0});
    }

    SECTION("LP fc=10kHz")
    {
        runLPSweep<2>(sr, 10000.0, {500.0, 2000.0, 5000.0, 10000.0, 15000.0, 20000.0});
        runLPSweep<4>(sr, 10000.0, {500.0, 2000.0, 5000.0, 10000.0, 15000.0, 20000.0});
        runLPSweep<6>(sr, 10000.0, {500.0, 2000.0, 5000.0, 10000.0, 15000.0, 20000.0});
    }

    SECTION("HP fc=500Hz")
    {
        runHPSweep<2>(sr, 500.0, {100.0, 250.0, 500.0, 1000.0, 4000.0, 10000.0});
        runHPSweep<4>(sr, 500.0, {100.0, 250.0, 500.0, 1000.0, 4000.0, 10000.0});
        runHPSweep<6>(sr, 500.0, {100.0, 250.0, 500.0, 1000.0, 4000.0, 10000.0});
    }

    SECTION("HP fc=8kHz")
    {
        runHPSweep<2>(sr, 8000.0, {500.0, 2000.0, 4000.0, 8000.0, 12000.0, 20000.0});
        runHPSweep<4>(sr, 8000.0, {500.0, 2000.0, 4000.0, 8000.0, 12000.0, 20000.0});
        runHPSweep<6>(sr, 8000.0, {500.0, 2000.0, 4000.0, 8000.0, 12000.0, 20000.0});
    }
}

TEST_CASE("Butterworth additional sample rate / cutoff sweeps at 96kHz")
{
    const double sr = 96000.0;

    SECTION("LP fc=1kHz")
    {
        runLPSweep<2>(sr, 1000.0, {100.0, 500.0, 1000.0, 2000.0, 4000.0});
        runLPSweep<4>(sr, 1000.0, {100.0, 500.0, 1000.0, 2000.0, 4000.0});
        runLPSweep<6>(sr, 1000.0, {100.0, 500.0, 1000.0, 2000.0, 4000.0});
    }

    SECTION("HP fc=2kHz")
    {
        runHPSweep<2>(sr, 2000.0, {200.0, 1000.0, 2000.0, 4000.0, 10000.0});
        runHPSweep<4>(sr, 2000.0, {200.0, 1000.0, 2000.0, 4000.0, 10000.0});
        runHPSweep<6>(sr, 2000.0, {200.0, 1000.0, 2000.0, 4000.0, 10000.0});
    }
}

TEST_CASE("Butterworth stereo: independent left/right responses")
{
    const double sr = 96000.0;
    const double fc = 5000.0;

    SECTION("LP N=4: L in passband (1kHz), R in stopband (20kHz)")
    {
        sst::filters::ButterworthLP<4> f;
        f.setCutoffAndSampleRate(fc, sr);
        auto [dbL, dbR] = measureStereoDb(f, 1000.0, 20000.0, sr);
        INFO("L=" << dbL << " R=" << dbR);
        REQUIRE(dbL == Approx(0.0).margin(0.2));
        REQUIRE(dbR == Approx(expectedLPDb(20000.0, fc, sr, 4)).margin(0.5));
    }

    SECTION("HP N=4: L in stopband (1kHz), R in passband (20kHz)")
    {
        sst::filters::ButterworthHP<4> f;
        f.setCutoffAndSampleRate(fc, sr);
        auto [dbL, dbR] = measureStereoDb(f, 1000.0, 20000.0, sr);
        INFO("L=" << dbL << " R=" << dbR);
        REQUIRE(dbL == Approx(expectedHPDb(1000.0, fc, sr, 4)).margin(0.5));
        REQUIRE(dbR == Approx(0.0).margin(0.2));
    }

    SECTION("LP N=6: both at cutoff but different sides")
    {
        sst::filters::ButterworthLP<6> f;
        f.setCutoffAndSampleRate(fc, sr);
        // L=2k (passband), R=10k (one octave above fc, deep in stopband)
        auto [dbL, dbR] = measureStereoDb(f, 2000.0, 10000.0, sr);
        INFO("L=" << dbL << " R=" << dbR);
        REQUIRE(dbL == Approx(expectedLPDb(2000.0, fc, sr, 6)).margin(0.3));
        REQUIRE(dbR == Approx(expectedLPDb(10000.0, fc, sr, 6)).margin(0.5));
    }

    SECTION("HP N=2: L=500Hz, R=8kHz at sr=88.2kHz, fc=2kHz")
    {
        const double sr2 = 88200.0;
        const double fc2 = 2000.0;
        sst::filters::ButterworthHP<2> f;
        f.setCutoffAndSampleRate(fc2, sr2);
        auto [dbL, dbR] = measureStereoDb(f, 500.0, 8000.0, sr2);
        INFO("L=" << dbL << " R=" << dbR);
        REQUIRE(dbL == Approx(expectedHPDb(500.0, fc2, sr2, 2)).margin(0.5));
        REQUIRE(dbR == Approx(expectedHPDb(8000.0, fc2, sr2, 2)).margin(0.3));
    }
}
