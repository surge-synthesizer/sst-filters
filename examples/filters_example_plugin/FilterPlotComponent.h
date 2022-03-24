#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <sst/filters/FilterPlotter.h>

class FilterPlotComponent : public juce::Component,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::Timer
{
  public:
    explicit FilterPlotComponent(juce::AudioProcessorValueTreeState &vts);
    ~FilterPlotComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    void timerCallback() override;
    void parameterChanged(const juce::String &parameterID, float newValue) override;
    void updatePlotPath();
    void drawPlotBackground(juce::Graphics &g);

    juce::AudioProcessorValueTreeState &vts;
    std::atomic<float> *freqHzParam = nullptr;
    std::atomic<float> *resParam = nullptr;
    std::atomic<float> *filterTypeParam = nullptr;
    std::atomic<float> *filterSubTypeParam = nullptr;

    sst::filters::FilterPlotter filterPlotter;
    juce::Path plotPath;
    std::atomic_bool pathNeedsUpdate{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPlotComponent)
};
