/*
    DryWet.cpp — see header.
*/
#include "DryWet.h"

void DryWet::prepare (double sampleRate, int blockSize)
{
    // TODO: implement
    juce::ignoreUnused (sampleRate, blockSize);
}

void DryWet::process (juce::AudioBuffer<float>& buffer)
{
    // TODO: implement
    juce::ignoreUnused (buffer);
}

void DryWet::reset()
{
    // TODO: implement
}

void DryWet::applyCrossfade (juce::SmoothedValue<float>& mix,
                             juce::AudioBuffer<float>& wetBuffer,
                             const juce::AudioBuffer<float>& dryBuffer)
{
    const int numCh = juce::jmin (wetBuffer.getNumChannels(), dryBuffer.getNumChannels());
    const int numSm = juce::jmin (wetBuffer.getNumSamples(), dryBuffer.getNumSamples());

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* wet = wetBuffer.getWritePointer (ch);
        const float* dry = dryBuffer.getReadPointer (ch);

        for (int n = 0; n < numSm; ++n)
        {
            const float m = mix.getNextValue();
            const float c = std::cos (m * juce::MathConstants<float>::halfPi);
            const float s = std::sin (m * juce::MathConstants<float>::halfPi);
            wet[n] = dry[n] * c + wet[n] * s;
        }
    }
}