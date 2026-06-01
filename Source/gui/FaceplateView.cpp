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

    addAndMakeVisible (transferCurve);
    addAndMakeVisible (harmonicBars);
    addAndMakeVisible (driveKnob);
    // vuMeter / bypassLed: Phase 2b — not drawn over the photoreal chassis yet.
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

    // Map a 960x600 reference rect [x,y,w,h] onto the actual size.
    auto place = [sx, sy] (int rx, int ry, int rw, int rh)
    {
        return juce::Rectangle<int> (juce::roundToInt ((float) rx * sx),
                                     juce::roundToInt ((float) ry * sy),
                                     juce::roundToInt ((float) rw * sx),
                                     juce::roundToInt ((float) rh * sy));
    };

    // Authoritative anchors from Claude Design (faceplate-pro-anchors.json, 960x600).
    transferCurve.setBounds (place (57, 168, 240, 240));   // transfer_panel
    harmonicBars.setBounds  (place (705, 168, 240, 166));  // harmonics_panel
    driveKnob.setBounds     (place (501 - 115, 289 - 115, 230, 230)); // hero_knob_well, centre (501,289)
}
