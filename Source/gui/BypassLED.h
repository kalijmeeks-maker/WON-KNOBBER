/*
    BypassLED.h — glowing bypass indicator LED
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class BypassLED : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void setActive (bool shouldBeActive);

private:
    bool active { false };
};
