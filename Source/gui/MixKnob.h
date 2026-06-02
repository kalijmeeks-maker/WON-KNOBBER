/*
    MixKnob.h — thin wrapper Component owning a non-filmstrip rotary Slider for DRY/WET mix
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "KnobLookAndFeel.h"

class MixKnob : public juce::Component
{
public:
    MixKnob();
    ~MixKnob() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getSlider() { return slider; }

private:
    KnobLookAndFeel lnf;
    juce::Slider slider;
};
