/*
    PluginEditor.h — WonKnobberAudioProcessorEditor: 480x180 window hosting the drive control
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "gui/FaceplateView.h"
#include "gui/FlipTransition.h"
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

    // Animate the front<->rear flip (Design's 180° rotateY / 450ms transition). toRear=true flips to
    // the service panel; false flips back. No-op while a flip is already running.
    void startFlip(bool toRear);

    WonKnobberAudioProcessor& processorRef;
    FaceplateView faceplate;
    RearPanelView rear;      // flip-to-rear service panel (cab/neural override); hidden until flipped
    FlipTransition flipAnim; // front<->rear flip animation overlay; hidden except mid-transition
    double lastTickSec { 0.0 }; // ballistics dt — first tick falls back to 1/30 s

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WonKnobberAudioProcessorEditor)
};
