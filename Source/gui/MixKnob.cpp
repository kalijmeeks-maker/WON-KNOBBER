/*
    MixKnob.cpp — see header. Owns Slider + LNF (no filmstrip set → ellipse fallback per docs/gui.md).
*/
#include "MixKnob.h"

MixKnob::MixKnob()
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (0.0, 1.0, 0.0);
    slider.setLookAndFeel (&lnf);
    // No filmstrip set on this lnf → falls back to simple ellipse rotary (suitable for mix, not gem).

    addAndMakeVisible (slider);
}

MixKnob::~MixKnob()
{
    slider.setLookAndFeel (nullptr);
}

void MixKnob::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
    // Drawing delegated entirely to child Slider + KnobLookAndFeel.
    // TODO in scaffold; filled in integration if custom needed.
}

void MixKnob::resized()
{
    slider.setBounds (getLocalBounds());
}