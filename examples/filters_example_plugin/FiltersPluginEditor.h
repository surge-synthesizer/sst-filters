#pragma once

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
