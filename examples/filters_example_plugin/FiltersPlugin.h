#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <sst/filters.h>

namespace ParamTags
{
const juce::String freqTag = "freq_hz";
const juce::String resTag = "res";
const juce::String filterTypeTag = "filter_type";
const juce::String filterSubTypeTag = "filter_subtype";
} // namespace ParamTags

namespace ParamConversions
{
inline float freq_hz_to_note_num(float freqHz) { return 12.0f * std::log2(freqHz / 440.0f); }

inline auto getFilterType(std::atomic<float> *param)
{
    return static_cast<sst::filters::FilterType>((int)*param);
}

inline auto getFilterSubType(std::atomic<float> *param)
{
    return static_cast<sst::filters::FilterSubType>((int)*param);
}
} // namespace ParamConversions

class FiltersPlugin : public juce::AudioProcessor
{
  public:
    FiltersPlugin();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram(int) override { return; };
    const juce::String getProgramName(int) override { return juce::String(); };
    void changeProgramName(int, const juce::String &) override {}

    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;

    void getStateInformation(juce::MemoryBlock &data) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    auto &getVTS() { return vts; }

  private:
    std::atomic<float> *freqHzParam = nullptr;
    std::atomic<float> *resParam = nullptr;
    std::atomic<float> *filterTypeParam = nullptr;
    std::atomic<float> *filterSubTypeParam = nullptr;
    juce::AudioProcessorValueTreeState vts;

    struct FilterUnit
    {
        sst::filters::QuadFilterUnitState filterState{};

        static constexpr auto dbLength =
            sst::filters::utilities::MAX_FB_COMB + sst::filters::utilities::SincTable::FIRipol_N;
        float delayBufferData[4][dbLength]{};

        void reset()
        {
            for (int i = 0; i < 4; ++i)
            {
                std::fill(delayBufferData[i], delayBufferData[i] + dbLength, 0.0f);
                filterState.DB[i] = delayBufferData[i];
                filterState.active[i] = (int)0xffffffff;
                filterState.WP[i] = 0;
            }

            std::fill(filterState.R, &filterState.R[sst::filters::n_filter_registers],
                      _mm_setzero_ps());
        }
    };

    sst::filters::FilterType lastFilterType{};
    sst::filters::FilterSubType lastFilterSubType{};

    sst::filters::FilterCoefficientMaker<> coeffMaker;
    std::vector<FilterUnit> filterUnits;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiltersPlugin)
};
