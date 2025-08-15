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
#ifndef SSTFILTERS_EXAMPLES_FILTERS_EXAMPLE_PLUGIN_FILTERPLOTCOMPONENT_H
#define SSTFILTERS_EXAMPLES_FILTERS_EXAMPLE_PLUGIN_FILTERPLOTCOMPONENT_H

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

#endif // SSTFILTERS_EXAMPLES_FILTERS_EXAMPLE_PLUGIN_FILTERPLOTCOMPONENT_H
