/*
    NeuralModel.cpp — see header. Placeholder; no inference engine integrated yet.
*/
#include "NeuralModel.h"

void NeuralModel::prepare (double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;
}

void NeuralModel::process (juce::AudioBuffer<float>& buffer)
{
    juce::ignoreUnused (buffer);
    // TODO: run neural inference (real-time safe; pre-allocate all scratch in prepare()).
}

void NeuralModel::reset()
{
    // TODO: reset internal inference state.
}

void NeuralModel::loadModel (const juce::File& modelFile)
{
    juce::ignoreUnused (modelFile);
    // TODO: load ONNX model off the audio thread; set modelLoaded on success.
    modelLoaded = false;
}
