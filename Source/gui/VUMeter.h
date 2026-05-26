/*
    VUMeter.h — analog-style VU meter component (30 Hz refresh, amber->red gradient)
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VUMeter : public juce::Component,
                private juce::Timer
{
public:
    VUMeter();
    ~VUMeter() override;

    void paint (juce::Graphics& g) override;

    // Called from the message thread with a 0.0-1.0 level read from the processor.
    void setLevel (float newLevel);

private:
    void timerCallback() override;

    float level { 0.0f };
};
