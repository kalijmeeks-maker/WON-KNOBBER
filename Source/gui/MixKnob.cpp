/*
    MixKnob.cpp — see header. Reuses KnobLookAndFeel (no filmstrip → ellipse fallback);
    suitable for mix (no gem filmstrip asset).
*/
#include "MixKnob.h"

MixKnob::MixKnob()
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (0.0, 1.0, 0.0);
    slider.setLookAndFeel (&lnf);
    // No filmstrip set on this lnf instance → falls back to simple (now always circular via LNF square dest) ellipse rotary.

    addAndMakeVisible (slider);
}

MixKnob::~MixKnob()
{
    slider.setLookAndFeel (nullptr);
}

void MixKnob::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
    // Drawing delegated entirely to child Slider + KnobLookAndFeel (ellipse fallback).
    // No custom paint here.
}

void MixKnob::resized()
{
    slider.setBounds (getLocalBounds());
}
