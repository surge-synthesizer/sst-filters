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
#ifndef SSTFILTERS_EXAMPLES_FILTERS_EXAMPLE_PLUGIN_FILTERSPLUGINEDITOR_H
#define SSTFILTERS_EXAMPLES_FILTERS_EXAMPLE_PLUGIN_FILTERSPLUGINEDITOR_H

#include "FiltersPlugin.h"
#include "FilterPlotComponent.h"

class FiltersPluginEditor : public juce::AudioProcessorEditor,
                            private juce::AudioProcessorParameter::Listener
{
  public:
    explicit FiltersPluginEditor(FiltersPlugin &plugin);
    ~FiltersPluginEditor() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int, bool) override {}
    void updateSubTypeMenu(int typeChoice);

    FiltersPlugin &plugin;

    juce::Slider freqSlider, resSlider;
    juce::ComboBox typeBox, subTypeBox;
    juce::Label freqLabel, resLabel, typeLabel, subTypeLabel;

    juce::AudioParameterChoice *typeChoiceParam = nullptr;

    std::unique_ptr<juce::SliderParameterAttachment> freqAttachment, resAttachment;
    std::unique_ptr<juce::ComboBoxParameterAttachment> typeAttachment;

    class SubTypeComboBoxParameterAttachment;
    std::unique_ptr<SubTypeComboBoxParameterAttachment> subTypeAttachment;

    FilterPlotComponent plotComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FiltersPluginEditor)
};

#endif // SSTFILTERS_EXAMPLES_FILTERS_EXAMPLE_PLUGIN_FILTERSPLUGINEDITOR_H
