/*
    MixKnob.h — DRY/WET mix rotary (reuses KnobLookAndFeel without filmstrip for plain knob)
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

    // Bypass dim-state (§3.4): dull the value-arc like the hero knob (own LookAndFeel instance).
    void setBypassed (bool b) { lnf.setBypassed (b); slider.repaint(); }

private:
    KnobLookAndFeel lnf;
    juce::Slider slider;
};