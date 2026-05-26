/*
    NeuralModel.h — placeholder neural-inference stage (NEBULA-style character)
    WON-KNOBBER · part of the dsp layer
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class NeuralModel
{
public:
    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    // Loads ONNX weights (fixed I/O shape) on the message thread.
    void loadModel (const juce::File& modelFile);

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };
    bool modelLoaded { false };
    // TODO: hold the inference engine session/handle here.
};
