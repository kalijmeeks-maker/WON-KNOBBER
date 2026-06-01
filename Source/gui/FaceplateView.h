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
    void drawEngraved (juce::Graphics& g, const juce::String& text,
                       juce::Rectangle<int> area, float fontSize) const;

    KnobLookAndFeel knobLnf;
    juce::Slider driveKnob;
    VUMeter vuMeter;
    BypassLED bypassLed;

    // layout rects shared between resized() and paint()
    juce::Rectangle<int> knobArea, brandArea, satLabelArea, minMaxArea;
};
