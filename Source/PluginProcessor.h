/*
    PluginProcessor.h — WonKnobberAudioProcessor: owns the `drive` parameter + DSP chain
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/Convolution.h"
#include "dsp/DryWet.h"
#include "dsp/NeuralModel.h"
#include "dsp/Saturation.h"

class WonKnobberAudioProcessor : public juce::AudioProcessor
{
public:
    WonKnobberAudioProcessor();
    ~WonKnobberAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "WON-KNOBBER"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioParameterFloat* getDriveParameter() const noexcept { return drive; }
    juce::AudioParameterFloat* getMixParameter() const noexcept { return mix; }

private:
    juce::AudioParameterFloat* drive { nullptr }; // 0.0 - 1.0, default 0.5
    juce::AudioParameterFloat* mix   { nullptr }; // 0.0 - 1.0, default 1.0 (full wet for backwards compat)

    Saturation saturation;
    Convolution convolution;
    NeuralModel neuralModel;

    juce::AudioBuffer<float> dryBuffer;      // pre-saturation scratch (sized in prepareToPlay)
    juce::SmoothedValue<float> mixSmooth { 1.0f };
    DryWet dryWet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WonKnobberAudioProcessor)
};
