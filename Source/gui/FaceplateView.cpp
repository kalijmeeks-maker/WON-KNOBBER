/*
    FaceplateView.cpp — draws the embedded photoreal PRO chassis (960x600) and seats the
    hero filmstrip knob in the SATURATION well. Controls are placed in 960x600 reference
    coords and scaled to the actual size. Live instruments (transfer / harmonics / meters)
    are Phase 2.
*/
#include "FaceplateView.h"

#include "BinaryData.h"

FaceplateView::FaceplateView()
{
    faceplate = juce::ImageCache::getFromMemory (
        BinaryData::faceplate_pro_960x600_png, BinaryData::faceplate_pro_960x600_pngSize);

    knobLnf.setFilmstrip (juce::ImageCache::getFromMemory (
        BinaryData::knob_diamond_256_png, BinaryData::knob_diamond_256_pngSize));

    driveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    driveKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driveKnob.setRange (0.0, 1.0, 0.0);
    driveKnob.setLookAndFeel (&knobLnf);

    addAndMakeVisible (driveKnob);
    // vuMeter / bypassLed: Phase 2 — not drawn over the photoreal chassis yet.
}

FaceplateView::~FaceplateView()
{
    driveKnob.setLookAndFeel (nullptr);
}

void FaceplateView::paint (juce::Graphics& g)
{
    if (faceplate.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (faceplate, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll (juce::Colour (0xff141517));
    }
}

void FaceplateView::resized()
{
    const float sx = (float) getWidth()  / (float) kRefW;
    const float sy = (float) getHeight() / (float) kRefH;

    // SATURATION knob well, measured from the render: ~290px square centred at (504, 282).
    const auto knobRef = juce::Rectangle<int> (504 - 145, 282 - 145, 290, 290);
    driveKnob.setBounds (juce::Rectangle<int> (
        juce::roundToInt ((float) knobRef.getX()     * sx),
        juce::roundToInt ((float) knobRef.getY()     * sy),
        juce::roundToInt ((float) knobRef.getWidth()  * sx),
        juce::roundToInt ((float) knobRef.getHeight() * sy)));
}
