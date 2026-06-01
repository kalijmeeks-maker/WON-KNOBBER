/*
    FaceplateView.cpp — root container: chassis paint + engraved labels, lays out knob/meter/LED.
    The knob renders from the embedded 120-frame Blender filmstrip via KnobLookAndFeel.
*/
#include "FaceplateView.h"

#include "BinaryData.h"

FaceplateView::FaceplateView()
{
    knobLnf.setFilmstrip (juce::ImageCache::getFromMemory (
        BinaryData::knob_anodize_black_256_png, BinaryData::knob_anodize_black_256_pngSize));

    driveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driveKnob.setRange (0.0, 1.0, 0.0);
    driveKnob.setLookAndFeel (&knobLnf);

    addAndMakeVisible (driveKnob);
    addAndMakeVisible (vuMeter);
    addAndMakeVisible (bypassLed);
}

FaceplateView::~FaceplateView()
{
    driveKnob.setLookAndFeel (nullptr);
}

void FaceplateView::drawEngraved (juce::Graphics& g, const juce::String& text,
                                  juce::Rectangle<int> area, float fontSize) const
{
    // Fake wide tracking by spacing the glyphs; sell the recess with a dark drop + light face.
    juce::String spaced;
    for (int i = 0; i < text.length(); ++i)
    {
        spaced += text.substring (i, i + 1);
        if (i < text.length() - 1)
            spaced += " ";
    }

    g.setFont (juce::Font (juce::FontOptions (fontSize).withStyle ("Bold")));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText (spaced, area.translated (0, 1), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xff9aa0a3)); // tone-on-tone brushed-metal label
    g.drawText (spaced, area, juce::Justification::centred, false);
}

void FaceplateView::paint (juce::Graphics& g)
{
    const auto bf = getLocalBounds().toFloat();

    // Dark brushed-metal chassis, single implied overhead light (top lighter -> bottom darker).
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35383b), bf.getCentreX(), bf.getY(),
                                             juce::Colour (0xff141517), bf.getCentreX(), bf.getBottom(), false));
    g.fillRect (bf);

    // Raised-hardware edges: 1px top highlight, 1px bottom shadow.
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (bf.withHeight (1.0f));
    g.setColour (juce::Colours::black.withAlpha (0.60f));
    g.fillRect (bf.withTop (bf.getBottom() - 1.0f));

    drawEngraved (g, "WON KNOBBER", brandArea, 15.0f);
    drawEngraved (g, "SATURATION", satLabelArea, 11.0f);

    auto mm = minMaxArea;
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.setColour (juce::Colour (0xff7a7e80));
    g.drawText ("MIN", mm.removeFromLeft (mm.getWidth() / 3), juce::Justification::centredLeft, false);
    g.drawText ("MAX", mm.removeFromRight (mm.getWidth() / 2), juce::Justification::centredRight, false);
}

void FaceplateView::resized()
{
    auto area = getLocalBounds().reduced (16);

    auto top = area.removeFromTop (30);
    bypassLed.setBounds (top.removeFromRight (22).withSizeKeepingCentre (18, 18));
    brandArea = top;

    auto bottom = area.removeFromBottom (74);
    vuMeter.setBounds (bottom.withSizeKeepingCentre (juce::jmin (220, bottom.getWidth()),
                                                     juce::jmin (64, bottom.getHeight())));

    satLabelArea = area.removeFromTop (20);
    minMaxArea   = area.removeFromBottom (16);

    const int side = juce::jmin (area.getWidth(), area.getHeight());
    knobArea = juce::Rectangle<int> (0, 0, side, side).withCentre (area.getCentre());
    driveKnob.setBounds (knobArea);
}
