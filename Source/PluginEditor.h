/*
    PluginEditor.h — WonKnobberAudioProcessorEditor: 480x180 window hosting the drive control
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "gui/FaceplateView.h"
#include "gui/RearPanelView.h"

class WonKnobberAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit WonKnobberAudioProcessorEditor (WonKnobberAudioProcessor&);
    ~WonKnobberAudioProcessorEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override; // pushes Drive (incl. host automation) to the scopes

    WonKnobberAudioProcessor& processorRef;
    FaceplateView faceplate;
    RearPanelView rear; // flip-to-rear service panel (cab/neural override); hidden until flipped
    double lastTickSec { 0.0 }; // ballistics dt — first tick falls back to 1/30 s

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WonKnobberAudioProcessorEditor)
};
