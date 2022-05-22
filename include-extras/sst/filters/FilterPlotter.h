#ifndef SST_FILTERS_FILTERPLOTTER_H
#define SST_FILTERS_FILTERPLOTTER_H

#include <sst/filters.h>

// This header must be used inside a JUCE environment
#include <juce_dsp/juce_dsp.h>

namespace sst
{
namespace filters
{

struct FilterPlotParameters
{
    float sampleRate = 96000.0f;
    float startFreqHz = 20.0f;
    float endFreqHz = 20000.0f;
    float inputAmplitude = 1.0f / juce::MathConstants<float>::sqrt2;
    float freqSmoothOctaves = 1.0f / 12.0f;
};

class FilterPlotter
{
  public:
    explicit FilterPlotter(int fftOrder = 15) : fft(fftOrder), fftSize(1 << fftOrder) {}

    std::pair<std::vector<float>, std::vector<float>>
    plotFilterMagnitudeResponse(sst::filters::FilterType filterType,
                                sst::filters::FilterSubType filterSubType, float pitch, float res,
                                const FilterPlotParameters& params = {})
    {
        // set up input sweep
        std::vector<float> sweepBuffer(fftSize, 0.0f);
        generateLogSweep(sweepBuffer.data(), fftSize, params);

        // set up filter
        float delayBuffer[4][sst::filters::utilities::MAX_FB_COMB +
                       sst::filters::utilities::SincTable::FIRipol_N];
        auto filterState = sst::filters::QuadFilterUnitState{};
        for (auto i=0; i<4; ++i)
        {
            filterState.DB[i] = &(delayBuffer[i][0]);
        }
        auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(filterType, filterSubType);

        sst::filters::FilterCoefficientMaker coefMaker;
        coefMaker.setSampleRateAndBlockSize(params.sampleRate, 512);
        coefMaker.MakeCoeffs(pitch, res, filterType, filterSubType, nullptr, false);
        coefMaker.updateState(filterState);

        // process filter
        std::vector<float> filterBuffer(fftSize, 0.0f);
        if (filterUnitPtr != nullptr)
            runFilter (filterState, filterUnitPtr, sweepBuffer.data(), filterBuffer.data(), fftSize);
        else
            std::copy (sweepBuffer.begin(), sweepBuffer.end(), filterBuffer.begin());

        auto magResponseDB = computeFrequencyResponse(sweepBuffer.data(), filterBuffer.data(), fftSize);
        auto magResponseDBSmoothed = freqSmooth(magResponseDB.data(), (int) magResponseDB.size(), params.freqSmoothOctaves);
        auto freqAxis = fftFreqs((int) magResponseDB.size(), 1.0f / params.sampleRate);

        return { std::move (freqAxis), std::move (magResponseDBSmoothed) };
    }

  private:
    static void generateLogSweep(float *buffer, int nSamples, const FilterPlotParameters& params)
    {
        const auto beta = (float)nSamples / std::log(params.endFreqHz / params.startFreqHz);

        for (int i = 0; i < nSamples; i++)
        {
            float phase = 2.0f * (float)M_PI * beta * params.startFreqHz *
                          (std::pow(params.endFreqHz / params.startFreqHz, (float)i / (float)nSamples) - 1.0f);

            buffer[i] = params.inputAmplitude * std::sin((phase + (float)M_PI / 180.0f) / params.sampleRate);
        }
    }

    static void runFilter (sst::filters::QuadFilterUnitState &filterState, sst::filters::FilterUnitQFPtr &filterUnitPtr, const float* inBuffer, float* outBuffer, int numSamples)
    {
        // reset filter state
        std::fill (filterState.R, &filterState.R[sst::filters::n_filter_registers], _mm_setzero_ps());

        for (int i=0; i<4; ++i)
        {
            filterState.WP[i] = 0;
            filterState.active[i] = 0;
        }
        filterState.active[0] = 0xFFFFFFFF;

        for (int i = 0; i < numSamples; ++i)
        {
            auto yVec = filterUnitPtr(&filterState, _mm_set_ps1(inBuffer[i]));

            float yArr alignas(16)[4];
            _mm_store_ps (yArr, yVec);
            outBuffer[i] = yArr[0];
        }
    };

    std::vector<float> computeFrequencyResponse(float* sweepBuffer, float* filterBuffer, int numSamples)
    {
        const auto fftDataSize = numSamples * 2;
        std::vector<float> sweepFFT (fftDataSize, 0.0f);
        std::copy (sweepBuffer, sweepBuffer + numSamples, sweepFFT.begin());
        fft.performFrequencyOnlyForwardTransform (sweepFFT.data(), true);

        std::vector<float> filtFFT (fftDataSize, 0.0f);
        std::copy (filterBuffer, filterBuffer + numSamples, filtFFT.begin());
        fft.performFrequencyOnlyForwardTransform (filtFFT.data(), true);

        const auto fftOutSize = numSamples / 2 + 1;
        std::vector<float> magnitudeResponseDB (fftOutSize, 0.0f);
        for (int i = 0; i < fftOutSize; ++i)
            magnitudeResponseDB[i] = juce::Decibels::gainToDecibels(filtFFT[i] / sweepFFT[i]);

        return magnitudeResponseDB;
    }

    static std::vector<float> fftFreqs(int N, float T)
    {
        auto val = 0.5f / ((float) N * T);

        std::vector<float> results (N, 0.0f);
        std::iota (results.begin(), results.end(), 0.0f);
        std::transform(results.begin(), results.end(), results.begin(), [val] (auto x) { return x * val; });

        return results;
    }

    static std::vector<float> freqSmooth (const float* data, int numSamples, float smFactor = 1.0f / 24.0f)
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

    juce::dsp::FFT fft;
    const int fftSize;
};

} // namespace filters
} // namespace sst

#endif // SST_FILTERS_FILTERPLOTTER_H
