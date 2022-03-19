#include "FiltersPlugin.h"
#include "FiltersPluginEditor.h"

namespace
{

} // namespace

FiltersPlugin::FiltersPlugin()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      vts(*this, nullptr, juce::Identifier("Parameters"), createParameters())
{
    using namespace ParamTags;
    freqHzParam = vts.getRawParameterValue (freqTag);
    resParam = vts.getRawParameterValue (resTag);
    filterTypeParam = vts.getRawParameterValue (filterTypeTag);
    filterSubTypeParam = vts.getRawParameterValue (filterSubTypeTag);
}

juce::AudioProcessorValueTreeState::ParameterLayout FiltersPlugin::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    using VTSParam = juce::AudioProcessorValueTreeState::Parameter;
    using namespace ParamTags;

    juce::NormalisableRange freqRange{20.0f, 20000.0f};
    freqRange.setSkewForCentre(2000.0f);
    params.push_back(std::make_unique<VTSParam>(
        freqTag, "Frequency", juce::String(), freqRange, 1000.0f,
        [](float freqVal) {
            if (freqVal <= 1000.0f)
                return juce::String(freqVal, 2, false) + " Hz";

            return juce::String(freqVal / 1000.0f, 2, false) + " kHz";
        },
        [](const juce::String &s) {
            auto freqVal = s.getFloatValue();

            if (s.getLastCharacter() == 'k')
                freqVal *= 1000.0f;

            return freqVal;
        }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        resTag, "Resonance", juce::NormalisableRange{0.0f, 1.0f}, 0.5f));

    juce::StringArray filterTypeChoices;
    for (const auto& filter_type_name : sst::filters::filter_type_names)
        filterTypeChoices.add(filter_type_name);

    params.push_back(std::make_unique<juce::AudioParameterChoice> (filterTypeTag, "Filter Type", filterTypeChoices, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>(filterSubTypeTag, "Filter Sub-Type", 0, sst::filters::FilterSubType::st_tripole_HHH3, 0));

    return {params.begin(), params.end()};
}

bool FiltersPlugin::isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const
{
    // only supports mono and stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // input and output layout must be the same
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void FiltersPlugin::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    coeffMaker.setSampleRateAndBlockSize((float)sampleRate, samplesPerBlock);

    filterUnits.resize(getMainBusNumInputChannels());
    for (auto &filt : filterUnits)
        filt.reset();

    lastFilterType = ParamConversions::getFilterType (filterTypeParam);
    lastFilterSubType = ParamConversions::getFilterSubType (filterSubTypeParam);
}

void FiltersPlugin::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    const auto filterType = ParamConversions::getFilterType(filterTypeParam);
    const auto filterSubType = ParamConversions::getFilterSubType(filterSubTypeParam);

    if (filterType != lastFilterType || filterSubType != lastFilterSubType)
    {
        lastFilterType = filterType;
        lastFilterSubType = filterSubType;

        for (auto &filt : filterUnits)
            filt.reset();
    }

    auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(filterType, filterSubType);
    coeffMaker.MakeCoeffs(ParamConversions::freq_hz_to_note_num (*freqHzParam), *resParam, filterType, filterSubType, nullptr, false);

    if (filterUnitPtr == nullptr)
        return; // no filter to process!

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* x = buffer.getWritePointer (ch);

        auto& filter = filterUnits[ch];
        coeffMaker.updateState (filter.filterState);

        for (int n = 0; n < numSamples; ++n)
        {
            auto yVec = filterUnitPtr(&filter.filterState, _mm_set_ps1(x[n]));

            float yArr alignas(16)[4];
            _mm_store_ps (yArr, yVec);
            x[n] = yArr[0];
        }

    }

    coeffMaker.updateCoefficients(filterUnits[0].filterState);
}

juce::AudioProcessorEditor *FiltersPlugin::createEditor()
{
    return new FiltersPluginEditor (*this);
}

void FiltersPlugin::getStateInformation(juce::MemoryBlock &data)
{
    auto state = vts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, data);
}

void FiltersPlugin::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (vts.state.getType()))
            vts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new FiltersPlugin(); }