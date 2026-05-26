/*
    VUMeter.cpp — see header. Gradient draw stub; VU ballistics TODO.
*/
#include "VUMeter.h"

VUMeter::VUMeter()
{
    startTimerHz (30);
}

VUMeter::~VUMeter()
{
    stopTimer();
}

void VUMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient grad (juce::Colour (0xffffc24b), bounds.getBottomLeft(),
                               juce::Colour (0xffe2493b), bounds.getTopLeft(), false);
    g.setGradientFill (grad);
    // TODO: fill proportionally to `level` with proper VU ballistics.
    g.fillRect (bounds.removeFromBottom (bounds.getHeight() * level));
}

void VUMeter::setLevel (float newLevel)
{
    level = juce::jlimit (0.0f, 1.0f, newLevel);
}

void VUMeter::timerCallback()
{
    repaint();
}
