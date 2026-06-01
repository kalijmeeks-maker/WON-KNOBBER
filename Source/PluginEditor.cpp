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
