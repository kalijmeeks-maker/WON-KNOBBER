/*
    PluginProcessor.h — WonKnobberAudioProcessor: owns the `drive` parameter + DSP chain
    WON-KNOBBER · part of the core layer
*/
#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/Convolution.h"
#include "dsp/NeuralModel.h"
#include "dsp/Saturation.h"

class WonKnobberAudioProcessor : public juce::AudioProcessor
{
public:
    // GUI peak-meter snapshot — linear magnitudes (0..~1), pre- and post-chain,
    // per stereo channel. Mono streams populate L only and leave R = 0.
    struct LevelSnapshot
    {
        float inL  = 0.0f, inR  = 0.0f;
        float outL = 0.0f, outR = 0.0f;
    };

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

    // "Choose your stone" — persistent UI state (NOT an automatable audio param;
    // wrong shape for a host's automation lane). Round-tripped via
    // get/setStateInformation. Default = "diamond".
    juce::String getCurrentVariant() const                  { return currentVariant; }
    void         setCurrentVariant (const juce::String& v)  { currentVariant = v; }

    // Atomically read the latest per-block linear peaks across all 4 channels and
    // reset the audio-thread accumulators to 0. Called from the GUI timer (~30 Hz);
    // every block's peak is captured because reads of 0 between blocks just leave
    // the accumulator empty for the next merge. Lock-free.
    LevelSnapshot consumeMeterPeaks() noexcept;

private:
    juce::AudioParameterFloat* drive { nullptr }; // 0.0 - 1.0, default 0.5
    juce::String currentVariant { "diamond" };    // persisted in plugin state

    Saturation saturation;
    Convolution convolution;
    NeuralModel neuralModel;

    // RT-safe peak accumulators (audio thread compare-and-max, GUI thread exchanges
    // with 0). Linear magnitudes; the GUI does the dB conversion + ballistics.
    std::atomic<float> peakInL  { 0.0f }, peakInR  { 0.0f };
    std::atomic<float> peakOutL { 0.0f }, peakOutR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WonKnobberAudioProcessor)
};
