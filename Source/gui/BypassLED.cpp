/*
    BypassLED.cpp — see header. Glow draw stub.
*/
#include "BypassLED.h"

void BypassLED::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    const auto colour = active ? juce::Colour (0xff2fbfb0) : juce::Colour (0xff3a3d3e);
    g.setColour (colour);
    g.fillEllipse (bounds);
    // TODO: add outer glow when active.
}

void BypassLED::setActive (bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}
