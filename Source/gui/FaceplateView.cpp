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
    addAndMakeVisible (mixKnob);
    // vuMeter / bypassLed: Phase 2 — not drawn over the photoreal chassis yet.
    // mixKnob sets up its own Slider + KnobLookAndFeel (non-filmstrip) internally.
}

FaceplateView::~FaceplateView()
{
    driveKnob.setLookAndFeel (nullptr);
    // mixKnob's internal LNF (and its slider) cleaned in MixKnob::~ ; no shared LNF.
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

    // SATURATION knob — authoritative anchor from Claude Design: centre (501, 289), draw ~230px.
    const auto knobRef = juce::Rectangle<int> (501 - 115, 289 - 115, 230, 230);
    driveKnob.setBounds (juce::Rectangle<int> (
        juce::roundToInt ((float) knobRef.getX()     * sx),
        juce::roundToInt ((float) knobRef.getY()     * sy),
        juce::roundToInt ((float) knobRef.getWidth()  * sx),
        juce::roundToInt ((float) knobRef.getHeight() * sy)));

    // MIX (DRY/WET) knob — from /.../faceplate-pro-anchors.json "mix_knob" rect [758,346,134,110] centre [825,392].
    // Scaled exactly like driveKnob. LNF fallback now forces square dest (see KnobLookAndFeel.cpp) so always circular even in non-square well.
    const auto mixRef = juce::Rectangle<int> (758, 346, 134, 110);
    mixKnob.setBounds (juce::Rectangle<int> (
        juce::roundToInt ((float) mixRef.getX()     * sx),
        juce::roundToInt ((float) mixRef.getY()     * sy),
        juce::roundToInt ((float) mixRef.getWidth()  * sx),
        juce::roundToInt ((float) mixRef.getHeight() * sy)));
}
