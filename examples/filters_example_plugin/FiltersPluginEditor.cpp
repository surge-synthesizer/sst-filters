#include "FiltersPluginEditor.h"

class FiltersPluginEditor::SubTypeComboBoxParameterAttachment : private juce::ComboBox::Listener
{
  public:
    SubTypeComboBoxParameterAttachment(juce::RangedAudioParameter &param, juce::ComboBox &c,
                                       juce::UndoManager *um = nullptr)
        : comboBox(c), attachment(
                           param, [this](float f) { setValue(f); }, um)
    {
        sendInitialUpdate();
        comboBox.addListener(this);
    }

    ~SubTypeComboBoxParameterAttachment() override { comboBox.removeListener(this); }

    void sendInitialUpdate() { attachment.sendInitialUpdate(); }

  private:
    void setValue(float newValue)
    {
        const auto numItems = comboBox.getNumItems();
        int index = 0;

        if (numItems > 1)
            index = juce::jlimit(0, numItems - 1, (int)newValue);

        if (index == comboBox.getSelectedItemIndex())
            return;

        const juce::ScopedValueSetter<bool> svs(ignoreCallbacks, true);
        comboBox.setSelectedItemIndex(index, juce::sendNotificationSync);
    }

    void comboBoxChanged(juce::ComboBox *) override
    {
        if (ignoreCallbacks)
            return;

        const auto selected = (float)comboBox.getSelectedItemIndex();
        attachment.setValueAsCompleteGesture(selected);
    }

    juce::ComboBox &comboBox;
    juce::ParameterAttachment attachment;
    bool ignoreCallbacks = false;
};

FiltersPluginEditor::FiltersPluginEditor(FiltersPlugin &p)
    : juce::AudioProcessorEditor(p), plugin(p), plotComponent(p.getVTS())
{
    auto setupSlider = [this](auto &slider, auto &attachment, const juce::String &paramTag,
                              const juce::String &name, auto &label) {
        label.setText(name, juce::dontSendNotification);
        label.attachToComponent(&slider, true);
        label.setJustificationType(juce::Justification::right);

        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxRight, false, 70, 20);
        addAndMakeVisible(slider);

        auto *param = plugin.getVTS().getParameter(paramTag);
        attachment = std::make_unique<juce::SliderParameterAttachment>(*param, slider);
    };

    setupSlider(freqSlider, freqAttachment, ParamTags::freqTag, "Freq.", freqLabel);
    setupSlider(resSlider, resAttachment, ParamTags::resTag, "Res.", resLabel);

    auto setupComboBox = [this](auto &box, const juce::String &paramTag, const juce::String &name,
                                auto &label) {
        label.setText(name, juce::dontSendNotification);
        label.attachToComponent(&box, true);
        label.setJustificationType(juce::Justification::right);
        addAndMakeVisible(box);

        auto *param = plugin.getVTS().getParameter(paramTag);
        if (auto *choiceParam = dynamic_cast<juce::AudioParameterChoice *>(param))
        {
            box.addItemList(choiceParam->getAllValueStrings(), 1);
            box.setSelectedItemIndex(choiceParam->getIndex(), juce::dontSendNotification);
        }

        return param;
    };

    auto *typeParam = setupComboBox(typeBox, ParamTags::filterTypeTag, "Type", typeLabel);
    typeAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(*typeParam, typeBox);
    typeChoiceParam = dynamic_cast<juce::AudioParameterChoice *>(typeParam);
    typeChoiceParam->addListener(this);

    auto *subTypeParam =
        setupComboBox(subTypeBox, ParamTags::filterSubTypeTag, "Sub-Type", subTypeLabel);
    updateSubTypeMenu(typeChoiceParam->getIndex());
    subTypeAttachment =
        std::make_unique<SubTypeComboBoxParameterAttachment>(*subTypeParam, subTypeBox);

    addAndMakeVisible(plotComponent);

    setSize(1000, 500);
}

FiltersPluginEditor::~FiltersPluginEditor() { typeChoiceParam->removeListener(this); }

void fillStringArrayWithFilterSubTypes(juce::StringArray &strArray,
                                       sst::filters::FilterType subtype)
{
    auto fillArray = [&strArray](const auto &nameArray) {
        for (const auto &filter_subtype_name : nameArray)
            strArray.add(filter_subtype_name);
    };

    auto fillArray2D = [&strArray](const auto &nameArray1, const auto &nameArray2) {
        for (const auto &name_2 : nameArray2)
        {
            for (const auto &name_1 : nameArray1)
                strArray.add(juce::String(name_1) + ", " + name_2);
        }
    };

    const char nlf_saturators[3][16] = {"tanh", "Soft Clip", "OJD"};
    const char nlr_saturators[2][16] = {"tanh", "Soft Clip"};

    using sst::filters::FilterType;
    switch (subtype)
    {
    case FilterType::fut_lp12:
    case FilterType::fut_lp24:
    case FilterType::fut_bp12:
    case FilterType::fut_bp24:
    case FilterType::fut_hp12:
    case FilterType::fut_hp24:
        fillArray(sst::filters::fut_def_subtypes);
        break;
    case FilterType::fut_notch12:
    case FilterType::fut_notch24:
        fillArray(sst::filters::fut_notch_subtypes);
        break;
    case FilterType::fut_lpmoog:
    case FilterType::fut_diode:
        fillArray(sst::filters::fut_ldr_subtypes);
        break;
    case FilterType::fut_vintageladder:
        fillArray(sst::filters::fut_vintageladder_subtypes);
        break;
    case FilterType::fut_obxd_2pole_lp:
    case FilterType::fut_obxd_2pole_bp:
    case FilterType::fut_obxd_2pole_hp:
    case FilterType::fut_obxd_2pole_n:
        fillArray(sst::filters::fut_obxd_2p_subtypes);
        break;
    case FilterType::fut_obxd_4pole:
        fillArray(sst::filters::fut_obxd_4p_subtypes);
        break;
    case FilterType::fut_k35_lp:
    case FilterType::fut_k35_hp:
        fillArray(sst::filters::fut_k35_subtypes);
        break;
    case FilterType::fut_cutoffwarp_lp:
    case FilterType::fut_cutoffwarp_hp:
    case FilterType::fut_cutoffwarp_bp:
    case FilterType::fut_cutoffwarp_n:
    case FilterType::fut_cutoffwarp_ap:
        fillArray2D(sst::filters::fut_nlf_subtypes, nlf_saturators);
        break;
    case FilterType::fut_resonancewarp_lp:
    case FilterType::fut_resonancewarp_hp:
    case FilterType::fut_resonancewarp_bp:
    case FilterType::fut_resonancewarp_n:
    case FilterType::fut_resonancewarp_ap:
        fillArray2D(sst::filters::fut_nlf_subtypes, nlr_saturators);
        break;
    case FilterType::fut_tripole:
        fillArray2D(sst::filters::fut_tripole_subtypes, sst::filters::fut_tripole_output_stage);
        break;
    case FilterType::fut_comb_pos:
    case FilterType::fut_comb_neg:
        fillArray(sst::filters::fut_comb_subtypes);
        break;
    case FilterType::fut_apf:
    case FilterType::fut_SNH:
    case FilterType::fut_none:
    default:
        break;
    }
}

void FiltersPluginEditor::parameterValueChanged(int /*parameterIndex*/, float newValue)
{
    updateSubTypeMenu((int)typeChoiceParam->convertFrom0to1(newValue));
}

void FiltersPluginEditor::updateSubTypeMenu(int typeChoice)
{
    subTypeBox.clear();
    juce::StringArray filterSubTypeStrings;
    fillStringArrayWithFilterSubTypes(filterSubTypeStrings,
                                      static_cast<sst::filters::FilterType>(typeChoice));

    subTypeBox.addItemList(filterSubTypeStrings, 1);
    subTypeBox.setSelectedItemIndex(0);
}

void FiltersPluginEditor::paint(juce::Graphics &g) { g.fillAll(juce::Colours::grey); }

void FiltersPluginEditor::resized()
{
    auto bounds = getLocalBounds();

    plotComponent.setBounds(bounds.removeFromRight(700));

    constexpr int labelWidth = 55;
    bounds.removeFromLeft(labelWidth); // leave space for labels

    auto setComponentBounds = [labelWidth, &bounds](auto &comp, auto &label, int reduce = 0) {
        comp.setBounds(bounds.removeFromTop(50).reduced(reduce));
        label.setBounds(comp.getBounds().withX(0).withWidth(labelWidth));
    };

    setComponentBounds(freqSlider, freqLabel);
    setComponentBounds(resSlider, resLabel);
    setComponentBounds(typeBox, typeLabel, 10);
    setComponentBounds(subTypeBox, subTypeLabel, 10);
}
