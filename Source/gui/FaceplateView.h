/*
    FaceplateView.h — root container: draws the photoreal PRO chassis (960x600 PNG)
    and seats the hero filmstrip knob in the SATURATION well.
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
    // Design reference is the 960x600 PRO chassis; controls are placed in its coords.
    static constexpr int kRefW = 960;
    static constexpr int kRefH = 600;

    KnobLookAndFeel knobLnf;
    juce::Slider driveKnob;
    VUMeter vuMeter;     // Phase 2: live I/O meters (not shown yet)
    BypassLED bypassLed; // Phase 2: live bypass telltale (not shown yet)

    juce::Image faceplate; // embedded photoreal chassis (BinaryData)
};
