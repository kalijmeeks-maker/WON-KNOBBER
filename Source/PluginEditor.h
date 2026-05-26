/*
    PluginEditor.h — WonKnobberAudioProcessorEditor: 480x180 window hosting the drive control
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "gui/FaceplateView.h"

class WonKnobberAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WonKnobberAudioProcessorEditor (WonKnobberAudioProcessor&);
    ~WonKnobberAudioProcessorEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    WonKnobberAudioProcessor& processorRef;
    FaceplateView faceplate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WonKnobberAudioProcessorEditor)
};
