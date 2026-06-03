/*
    KnobLookAndFeel.cpp — filmstrip blit + amber value-arc + 7-stone variant map.
*/
#include "KnobLookAndFeel.h"

void KnobLookAndFeel::addVariant (const juce::String& stone, const juce::Image& strip)
{
    if (! strip.isValid())
        return;

    const int existing = variantNames.indexOf (stone);
    if (existing >= 0)
    {
        variants[(size_t) existing] = strip; // refresh
    }
    else if ((size_t) variantNames.size() < variants.size())
    {
        variants[(size_t) variantNames.size()] = strip;
        variantNames.add (stone);
    }

    // If this is the first variant, make it the active one.
    if (currentVariant.isEmpty())
    {
        currentVariant = stone;
        filmstrip = strip;
    }
}

void KnobLookAndFeel::setVariant (const juce::String& stone)
{
    const int idx = variantNames.indexOf (stone);
    if (idx < 0)
        return;
    currentVariant = stone;
    filmstrip = variants[(size_t) idx];
}

void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider);
    const auto bounds = juce::Rectangle<int> (x, y, width, height);
    const int  side   = juce::jmin (width, height);
    const auto centre = bounds.getCentre().toFloat();

    if (filmstrip.isValid())
    {
        const int frameHeight = filmstrip.getHeight() / numFrames;
        const int frameIndex = juce::jlimit (0, numFrames - 1,
                                             (int) std::round (sliderPosProportional * (numFrames - 1)));

        // Square dest centred in the slider bounds (keeps the knob's aspect ratio).
        const auto dest = juce::Rectangle<int> (0, 0, side, side).withCentre (bounds.getCentre());

        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (filmstrip,
                     dest.getX(), dest.getY(), dest.getWidth(), dest.getHeight(),
                     0, frameIndex * frameHeight, filmstrip.getWidth(), frameHeight);
    }
    else
    {
        // Placeholder until the filmstrip asset is wired in.
        g.setColour (juce::Colour (0xff2a2d2e));
        g.fillEllipse (bounds.toFloat());
        g.setColour (juce::Colour (0xffffb14e));
        g.drawEllipse (bounds.toFloat().reduced (2.0f), 2.0f);
    }

    // Amber value-arc around the knob (the diamond has no painted indicator).
    const float r       = (float) side * 0.5f * 0.96f;
    const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    juce::Path track, arc;
    track.addCentredArc (centre.x, centre.y, r, r, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    arc.addCentredArc   (centre.x, centre.y, r, r, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (bypassed ? juce::Colour (0xff6e6256).withAlpha (0.15f)  // §3.4 value_arc: dull grey-amber
                          : juce::Colour (0xffFE9A00));
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
