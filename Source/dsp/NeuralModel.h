/*
    NeuralModel.h — RTNeural-backed neural character stage (RT-safe, off by default).
    pImpl keeps RTNeural's heavy headers out of the rest of the build.
    WON-KNOBBER · part of the dsp layer
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <memory>

class NeuralModel
{
public:
    NeuralModel();
    ~NeuralModel();

    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    // Engage/disengage. Audio-thread-safe: process() is a bit-exact passthrough when off.
    void setEngaged (bool shouldEngage) noexcept;

    // Select the neural model by manifest slot id (NONE/TAPE/VALVE/TRANSISTOR/IRON).
    // Builds the embedded RTNeural model on the message thread, double-buffered + atomic swap.
    // NONE / unknown / unparseable -> no model (passthrough). Call on the message thread only.
    void setModel (const juce::String& modelId);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
