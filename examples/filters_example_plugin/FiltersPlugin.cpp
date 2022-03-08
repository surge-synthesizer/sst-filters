#include "FiltersPlugin.h"

namespace
{
const juce::String freqTag = "freq_hz";
const juce::String resTag = "res";
const juce::String filterTypeTag = "filter_type";

float freq_hz_to_note_num (float freqHz)
{
    return 12.0f * std::log2 (freqHz / 440.0f) + 69.0f;
}
} // namespace

FiltersPlugin::FiltersPlugin()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      vts(*this, nullptr, juce::Identifier("Parameters"), createParameters())
{
    freqHzParam = vts.getRawParameterValue (freqTag);
    resParam = vts.getRawParameterValue (resTag);
    filterTypeParam = vts.getRawParameterValue (filterTypeTag);
}

juce::AudioProcessorValueTreeState::ParameterLayout FiltersPlugin::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    using VTSParam = juce::AudioProcessorValueTreeState::Parameter;

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
        filterTypeChoices.add (filter_type_name);

    params.push_back (std::make_unique<juce::AudioParameterChoice> (filterTypeTag, "Filter Type", filterTypeChoices, 0));

    // @TODO: figure out parameter for filter sub-type

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
}

void FiltersPlugin::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    const auto filterType = static_cast<sst::filters::FilterType> ((int) *filterTypeParam);
    const auto filterSubType = sst::filters::st_SVF;

    auto filterUnitPtr = sst::filters::GetQFPtrFilterUnit(filterType, filterSubType);
    coeffMaker.MakeCoeffs(freq_hz_to_note_num (*freqHzParam), *resParam, filterType, filterSubType, nullptr, false);

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
}

juce::AudioProcessorEditor *FiltersPlugin::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
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