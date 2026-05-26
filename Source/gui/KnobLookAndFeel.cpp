/*
    KnobLookAndFeel.cpp — see header. Filmstrip blit + placeholder fallback. No real asset yet.
*/
#include "KnobLookAndFeel.h"

void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (rotaryStartAngle, rotaryEndAngle, slider);
    const auto bounds = juce::Rectangle<int> (x, y, width, height);

    if (filmstrip.isValid())
    {
        const int frameHeight = filmstrip.getHeight() / numFrames;
        const int frameIndex = juce::jlimit (0, numFrames - 1,
                                             (int) std::round (sliderPosProportional * (numFrames - 1)));
        // TODO: blit source rect (0, frameIndex*frameHeight, width, frameHeight) into bounds.
        juce::ignoreUnused (frameHeight, frameIndex);
    }
    else
    {
        // Placeholder until the filmstrip asset is wired in.
        g.setColour (juce::Colour (0xff2a2d2e));
        g.fillEllipse (bounds.toFloat());
        g.setColour (juce::Colour (0xffffb14e));
        g.drawEllipse (bounds.toFloat().reduced (2.0f), 2.0f);
    }
}
