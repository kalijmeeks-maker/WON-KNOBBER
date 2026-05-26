/*
    FaceplateView.h — root container: draws faceplate SVG, lays out knob/meter/LED
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "BypassLED.h"
#include "KnobLookAndFeel.h"
#include "VUMeter.h"

class FaceplateView : public juce::Component
{
public:
    FaceplateView();
    ~FaceplateView() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getDriveKnob() { return driveKnob; }
    VUMeter& getVUMeter() { return vuMeter; }
    BypassLED& getBypassLED() { return bypassLed; }

private:
    KnobLookAndFeel knobLnf;
    juce::Slider driveKnob;
    VUMeter vuMeter;
    BypassLED bypassLed;
    // TODO: std::unique_ptr<juce::Drawable> faceplateSvg loaded from BinaryData.
};
