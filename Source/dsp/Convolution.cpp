/*
    Convolution.cpp — see header. Stub: wiring only, no processing implemented.
*/
#include "Convolution.h"

void Convolution::prepare (double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) blockSize;
    spec.numChannels = 2;
    convolution.prepare (spec);
}

void Convolution::process (juce::AudioBuffer<float>& buffer)
{
    juce::ignoreUnused (buffer);
    // TODO: run convolution.process on a dsp::AudioBlock wrapping the buffer.
}

void Convolution::reset()
{
    convolution.reset();
}

void Convolution::loadIR (const juce::File& irFile)
{
    juce::ignoreUnused (irFile);
    // TODO: convolution.loadImpulseResponse(...) off the audio thread.
}
