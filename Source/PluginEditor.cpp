/*
    PluginEditor.cpp — see header. Wires the drive knob to the AudioParameterFloat.
*/
#include "PluginEditor.h"

#include <cmath>
#include <juce_core/juce_core.h>

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
            faceplate.setDrive ((float) knob.getValue());
        };
        faceplate.setDrive (driveParam->get());
    }

    // Restore the persisted stone (defaults to "diamond" on a fresh load) and
    // let the chip write any future picker change back to the processor.
    faceplate.setVariant (processorRef.getCurrentVariant());
    faceplate.onVariantPicked = [this] (const juce::String& stone)
    {
        processorRef.setCurrentVariant (stone);
    };

    setSize (960, 600);
    startTimerHz (30);
}

void WonKnobberAudioProcessorEditor::timerCallback()
{
    if (auto* driveParam = processorRef.getDriveParameter())
    {
        const float d = driveParam->get();
        auto& knob = faceplate.getDriveKnob();
        if (std::abs ((float) knob.getValue() - d) > 1.0e-4f)
            knob.setValue (d, juce::dontSendNotification); // reflect host automation on knob + arc
        faceplate.setDrive (d);
    }

    // Drain the audio-thread peak accumulators and feed them to the I/O meter
    // with a real elapsed-time delta so the hold/decay ballistics are frame-rate
    // independent (clamped to a sane upper bound in case the timer stalls).
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt   = lastTickSec > 0.0
                           ? (float) juce::jlimit (0.0, 0.25, now - lastTickSec)
                           : 1.0f / 30.0f;
    lastTickSec = now;

    const auto peaks = processorRef.consumeMeterPeaks();
    faceplate.pushLevels (peaks.inL, peaks.inR, peaks.outL, peaks.outR, dt);
}

void WonKnobberAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1b1e));
}

void WonKnobberAudioProcessorEditor::resized()
{
    faceplate.setBounds (getLocalBounds());
}
