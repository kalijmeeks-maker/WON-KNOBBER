/*
    FaceplateView.cpp — see header. SVG faceplate load TODO; layout implemented.
*/
#include "FaceplateView.h"

FaceplateView::FaceplateView()
{
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

void FaceplateView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1b1e)); // deep charcoal background
    // TODO: draw faceplate SVG (from BinaryData) scaled to fill.
}

void FaceplateView::resized()
{
    auto area = getLocalBounds().reduced (12);
    bypassLed.setBounds (area.removeFromLeft (24).removeFromTop (24));
    vuMeter.setBounds (area.removeFromRight (60));
    driveKnob.setBounds (area); // centered knob dominates the remaining space
}
