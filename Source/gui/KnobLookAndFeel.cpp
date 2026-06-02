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

        // Square dest centred in the slider bounds (keeps the knob's aspect ratio).
        const int side = juce::jmin (width, height);
        const auto dest = juce::Rectangle<int> (0, 0, side, side).withCentre (bounds.getCentre());

        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (filmstrip,
                     dest.getX(), dest.getY(), dest.getWidth(), dest.getHeight(),
                     0, frameIndex * frameHeight, filmstrip.getWidth(), frameHeight);
    }
    else
    {
        // Placeholder until the filmstrip asset is wired in.
        // Square dest centred (matches filmstrip path) so non-square bounds rects (e.g. mix_knob json rect [758,346,134,110])
        // still produce circular knob, not stretched oval. Premium round control always.
        const int side = juce::jmin (width, height);
        const auto dest = juce::Rectangle<int> (0, 0, side, side).withCentre (bounds.getCentre());

        g.setColour (juce::Colour (0xff2a2d2e));
        g.fillEllipse (dest.toFloat());
        g.setColour (juce::Colour (0xffffb14e));
        g.drawEllipse (dest.toFloat().reduced (2.0f), 2.0f);
    }
}
