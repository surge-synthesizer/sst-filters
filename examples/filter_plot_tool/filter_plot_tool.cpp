#include <matplotlibcpp.h>
#include <sst/filters.h>
#include "AudioFFT.h"

namespace plt = matplotlibcpp;

constexpr float sampleRate = 96000.0f;
constexpr int fftSize = 1 << 15;

void generateLogSweep(float *buffer, int nSamples, float startFreqHz, float endFreqHz,
                      float amplitude = 1.0f)
{
    const auto beta = (float)nSamples / std::log(endFreqHz / startFreqHz);

    for (int i = 0; i < nSamples; i++)
    {
        float phase = 2.0f * (float)M_PI * beta * startFreqHz *
                      (std::pow(endFreqHz / startFreqHz, (float)i / (float)nSamples) - 1.0f);

        buffer[i] = amplitude * std::sin((phase + (float)M_PI / 180.0f) / sampleRate);
    }
}

auto runFilter = [](auto &filterState, auto &filterUnitPtr, const float* inBuffer, float* outBuffer, int numSamples)
{
    // reset filter state
    std::fill (filterState.R, &filterState.R[sst::filters::n_filter_registers], _mm_setzero_ps());

    for (int i = 0; i < numSamples; ++i)
    {
        auto yVec = filterUnitPtr(&filterState, _mm_set_ps1(inBuffer[i]));

        float yArr alignas(16)[4];
        _mm_store_ps (yArr, yVec);
        outBuffer[i] = yArr[0];
    }
};

auto computeFrequencyResponse(float* sweepBuffer, float* filterBuffer, int numSamples)
{
    auto doFFT = [numSamples] (auto& input, auto& outReal, auto& outImag)
    {
        audiofft::AudioFFT fft;
        fft.init(numSamples);
        fft.fft(input, outReal.data(), outImag.data());
    };

    const auto fftOutSize = numSamples / 2 + 1;
    std::vector<float> sweepFFTReal (fftOutSize, 0.0f);
    std::vector<float> sweepFFTImag (fftOutSize, 0.0f);
    doFFT (sweepBuffer, sweepFFTReal, sweepFFTImag);

    std::vector<float> filtFFTReal (fftOutSize, 0.0f);
    std::vector<float> filtFFTImag (fftOutSize, 0.0f);
    doFFT (filterBuffer, filtFFTReal, filtFFTImag);

    std::vector<float> magnitudeResponseDB (fftOutSize, 0.0f);
    for (int i = 0; i < fftOutSize; ++i)
    {
        // abs(filterFFT / sweepFFT)
        auto numReal = sweepFFTReal[i] * filtFFTReal[i] + sweepFFTImag[i] * filtFFTImag[i];
        auto numImag = sweepFFTReal[i] * filtFFTImag[i] - sweepFFTImag[i] * filtFFTReal[i];
        auto denom = sweepFFTReal[i] * sweepFFTReal[i] + sweepFFTImag[i] * sweepFFTImag[i];
        auto magResponse = std::sqrt (numReal * numReal + numImag * numImag) / denom;

        // convert to DB
        magnitudeResponseDB[i] = 20.0f * std::log10(magResponse);
    }

    return magnitudeResponseDB;
}

auto fftFreqs(int N, float T)
{
    auto val = 1.0f / ((float) N * T);

    std::vector<float> results (N, 0.0f);
    std::iota (results.begin(), results.end(), 0.0f);
    std::transform(results.begin(), results.end(), results.begin(), [val] (auto x) { return x * val; });

    return results;
}

auto freqSmooth (const float* data, int numSamples, float smFactor = 1.0f / 24.0f)
{
    const auto s = smFactor > 1.0f ? smFactor : std::sqrt (std::pow (2.0f, smFactor));

    std::vector<float> smoothedVec (numSamples, 0.0f);
    for (int i = 0; i < numSamples; ++i)
    {
        auto i1 = std::max (int ((float) i / s), 0);
        auto i2 = std::min (int ((float) i * s) + 1, numSamples - 1);

        smoothedVec[i] = i2 > i1 ? std::accumulate(data + i1, data + i2, 0.0f) / float (i2 - i1) : 0.0f;
    }

    return smoothedVec;
}

void showHelp()
{
    std::cout << "SST Filter Plotting Tool:" << std::endl;
    std::cout << "Usage: filter_plot_tool <pitch> <res> <filter_type> <filter_subtype>" << std::endl;
    std::cout << "    Note that pitch is in units of MIDI note!" << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    if (argc > 5)
    {
        showHelp();
        return 1;
    }

    if (strcmp (argv[1], "--help") == 0)
    {
        showHelp();
        return 0;
    }

    float pitch = 69.0f;
    if (argc > 1)
        pitch = (float) std::atof(argv[1]);

    float res = 0.5f;
    if (argc > 2)
        res = (float) std::atof(argv[2]);

    auto filterType = sst::filters::FilterType::fut_lp24;
    if (argc > 3)
        filterType = static_cast<sst::filters::FilterType> (std::atoi(argv[3]));

    auto filterSubType = sst::filters::FilterSubType::st_SVF;
    if (argc > 4)
        filterSubType = static_cast<sst::filters::FilterSubType> (std::atoi(argv[4]));

    // set up input sweep
    std::vector<float> sweepBuffer(fftSize, 0.0f);
    generateLogSweep(sweepBuffer.data(), fftSize, 20.0f, 20000.0f);

    // set up filter
    auto filterState = sst::filters::QuadFilterUnitState{};
    auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(filterType, filterSubType);

    sst::filters::FilterCoefficientMaker coefMaker;
    coefMaker.setSampleRateAndBlockSize(sampleRate, 512);
    coefMaker.MakeCoeffs(pitch, res, filterType, filterSubType, nullptr, false);
    coefMaker.updateState(filterState);

    // process filter
    std::vector<float> filterBuffer(fftSize, 0.0f);
    runFilter (filterState, filterUnitPtr, sweepBuffer.data(), filterBuffer.data(), fftSize);

    // get magnitude response
    auto magResponseDB = computeFrequencyResponse(sweepBuffer.data(), filterBuffer.data(), fftSize);
    auto magResponseDBSmoothed = freqSmooth(magResponseDB.data(), (int) magResponseDB.size(), 1.0f / 3.0f);
    auto freqAxis = fftFreqs((int) magResponseDB.size(), 1.0f / sampleRate);

    // make plot
    plt::semilogx(freqAxis, magResponseDBSmoothed);

    plt::title("Filter Magnitude Response");
    plt::grid(true);

    auto maxDBVal = *std::max_element(magResponseDBSmoothed.begin(), magResponseDBSmoothed.end());
    plt::ylim(-60.0f, maxDBVal + 5.0f);
    plt::xlim(20.0f, 20000.0f);

    plt::show();

    return 0;
}
