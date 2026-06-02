/*
    PluginEditor.cpp — see header. Wires the drive knob to the AudioParameterFloat.
*/
#include "PluginEditor.h"

WonKnobberAudioProcessorEditor::WonKnobberAudioProcessorEditor (WonKnobberAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (faceplate);

    auto& knob = faceplate.getDriveKnob();
    if (auto* driveParam = processorRef.getDriveParameter())
    {
        knob.setValue (driveParam->get(), juce::dontSendNotification);
        knob.onValueChange = [this, &knob, driveParam]
        {
            *driveParam = (float) knob.getValue();
            juce::ignoreUnused (this);
        };
    }

    auto& mixKnob = faceplate.getMixKnob();
    if (auto* mixParam = processorRef.getMixParameter())
    {
        mixKnob.setValue (mixParam->get(), juce::dontSendNotification);
        mixKnob.onValueChange = [this, &mixKnob, mixParam]
        {
            *mixParam = (float) mixKnob.getValue();
            juce::ignoreUnused (this);
        };
    }

    // Note: knob values may desync on external state restore (e.g. host preset load) until param updates fire;
    // this is pre-existing pattern for drive; host usually pushes value change to UI.
    setSize (960, 600);
}

void WonKnobberAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1b1e));
}

void WonKnobberAudioProcessorEditor::resized()
{
    faceplate.setBounds (getLocalBounds());
}
