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

    void showPresetMenu();
    void handlePresetMenuResult(int);
    void applyLoadedStateToGui();

    // Flip front<->rear: snapshots the outgoing side, plays FlipTransition, shows the incoming side.
    void startFlip(bool toRear);

    WonKnobberAudioProcessor& processorRef;
    FaceplateView faceplate;
    RearPanelView rear;       // rear service panel (declared before flipAnim so flipAnim paints on top)
    FlipTransition flipAnim;  // front<->rear flip overlay; sits above both sides while animating
    double lastTickSec { 0.0 }; // ballistics dt — first tick falls back to 1/30 s

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WonKnobberAudioProcessorEditor)
};
