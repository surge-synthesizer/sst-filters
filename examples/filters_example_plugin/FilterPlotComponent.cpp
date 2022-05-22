#include "FilterPlotComponent.h"
#include "FiltersPlugin.h"

namespace
{
constexpr float lowFreq = 10.0f;
constexpr float highFreq = 24000.0f;
const auto scaleFreq = std::log(highFreq / lowFreq);
constexpr auto dbMin = -33.0f;
constexpr auto dbMax = 9.0f;
constexpr auto dbRange = dbMax - dbMin;
constexpr int labelHeight = 15;

float freqToX(float freq, int width)
{
    const auto xNorm = std::log(freq / lowFreq) / scaleFreq;
    return xNorm * (float)width;
}

float dbToY(float db, int height) { return (float)height * (dbMax - db) / dbRange; }
} // namespace

FilterPlotComponent::FilterPlotComponent(juce::AudioProcessorValueTreeState &vtState) : vts(vtState)
{
    vts.addParameterListener(ParamTags::freqTag, this);
    vts.addParameterListener(ParamTags::resTag, this);
    vts.addParameterListener(ParamTags::filterTypeTag, this);
    vts.addParameterListener(ParamTags::filterSubTypeTag, this);

    freqHzParam = vts.getRawParameterValue(ParamTags::freqTag);
    resParam = vts.getRawParameterValue(ParamTags::resTag);
    filterTypeParam = vts.getRawParameterValue(ParamTags::filterTypeTag);
    filterSubTypeParam = vts.getRawParameterValue(ParamTags::filterSubTypeTag);

    startTimerHz(32);
}

FilterPlotComponent::~FilterPlotComponent()
{
    vts.removeParameterListener(ParamTags::freqTag, this);
    vts.removeParameterListener(ParamTags::resTag, this);
    vts.removeParameterListener(ParamTags::filterTypeTag, this);
    vts.removeParameterListener(ParamTags::filterSubTypeTag, this);
}

void FilterPlotComponent::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::grey);
    drawPlotBackground(g);

    g.setColour(juce::Colours::yellow);
    g.strokePath(plotPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::JointStyle::curved));
}

void FilterPlotComponent::resized() { updatePlotPath(); }

void FilterPlotComponent::drawPlotBackground(juce::Graphics &g)
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto font = juce::Font{(float)labelHeight * 0.9f};
    g.setFont(font);

    // draw major lines
    for (float freq : {100.0f, 1000.0f, 10000.0f})
    {
        const auto xPos = freqToX(freq, width);
        juce::Line line{juce::Point{xPos, 0.0f}, juce::Point{xPos, (float)height}};
        g.drawLine(line);

        const auto over1000 = freq >= 1000.0f;
        const auto freqString =
            juce::String(over1000 ? freq / 1000.0f : freq) + (over1000 ? " kHz" : " Hz");
        const auto labelRect = juce::Rectangle{font.getStringWidth(freqString), labelHeight}
                                   .withBottomY(height)
                                   .withRightX((int)xPos);
        g.drawFittedText(freqString, labelRect, juce::Justification::bottom, 1);
    }

    for (float db : {-30.0f, -24.0f, -18.0f, -12.0f, -6.0f, 0.0f, 6.0f})
    {
        const auto yPos = dbToY(db, height);
        juce::Line line{juce::Point{0.0f, yPos}, juce::Point{(float)width, yPos}};
        g.drawLine(line);

        const auto dbString = juce::String(db) + " dB";
        const auto labelRect = juce::Rectangle{font.getStringWidth(dbString), labelHeight}
                                   .withBottomY((int)yPos)
                                   .withRightX(width);
        g.drawFittedText(dbString, labelRect, juce::Justification::right, 1);
    }

    // draw minor lines
    {
        juce::Graphics::ScopedSaveState gss{g};
        g.setOpacity(0.75f);
        for (float freqBase : {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f})
        {
            for (float freqMult : {10.0f, 100.0f, 1000.0f, 10000.0f})
            {
                const auto freq = freqBase * freqMult;
                const auto xPos = freqToX(freq, width);
                juce::Line line{juce::Point{xPos, 0.0f}, juce::Point{xPos, (float)height}};
                g.drawLine(line);
            }
        }

        for (float db : {-27.0f, -21.0f, -15.0f, -9.0f, -3.0f, 3.0f})
        {
            const auto yPos = dbToY(db, height);
            juce::Line line{juce::Point{0.0f, yPos}, juce::Point{(float)width, yPos}};
            g.drawLine(line);
        }
    }
}

void FilterPlotComponent::parameterChanged(const juce::String &, float)
{
    // We know that some parameter is being changed. We'll mark that here,
    // and after the message thread has propagated all the parameter
    // updates, we'll update the path in the timer callback.
    pathNeedsUpdate = true;
}

void FilterPlotComponent::timerCallback()
{
    bool isTrue = true;
    if (pathNeedsUpdate.compare_exchange_strong(isTrue, false))
        updatePlotPath();
}

void FilterPlotComponent::updatePlotPath()
{
    const auto width = getWidth();
    const auto height = getHeight();

    plotPath.clear();

    const auto filterType = ParamConversions::getFilterType(filterTypeParam);
    const auto filterSubType = ParamConversions::getFilterSubType(filterSubTypeParam);

    auto [freqAxis, magResponseDBSmoothed] = filterPlotter.plotFilterMagnitudeResponse(
        filterType, filterSubType, ParamConversions::freq_hz_to_note_num(*freqHzParam), *resParam);

    bool started = false;
    const auto nPoints = freqAxis.size();
    for (int i = 0; i < nPoints; ++i)
    {
        if (freqAxis[i] < lowFreq / 2.0f || freqAxis[i] > highFreq * 1.01f)
            continue;

        auto xDraw = freqToX(freqAxis[i], width);
        auto yDraw = dbToY(magResponseDBSmoothed[i], height);

        if (!started)
        {
            plotPath.startNewSubPath(xDraw, yDraw);
            started = true;
        }
        else
        {
            plotPath.lineTo(xDraw, yDraw);
        }
    }

    repaint();
}
