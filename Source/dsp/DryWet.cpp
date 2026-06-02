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

    // Outer loop must be over samples so the smoother advances ONCE per audio sample
    // across all channels. Original (per-channel outer) advanced the smoother numCh
    // times per sample period, leaving L and R seeing different mix coefficients
    // during transitions -> stereo correlation break + phasing during automation.
    constexpr int kMaxLocalCh = 8; // covers mono/stereo/quad/5.1 etc; jassert below catches any wider host.
    jassert (numCh <= kMaxLocalCh);
    float*       wetPtrs[kMaxLocalCh];
    const float* dryPtrs[kMaxLocalCh];
    for (int ch = 0; ch < numCh; ++ch)
    {
        wetPtrs[ch] = wetBuffer.getWritePointer (ch);
        dryPtrs[ch] = dryBuffer.getReadPointer  (ch);
    }

    for (int n = 0; n < numSm; ++n)
    {
        const float m = mix.getNextValue();
        const float c = std::cos (m * juce::MathConstants<float>::halfPi);
        const float s = std::sin (m * juce::MathConstants<float>::halfPi);
        for (int ch = 0; ch < numCh; ++ch)
            wetPtrs[ch][n] = dryPtrs[ch][n] * c + wetPtrs[ch][n] * s;
    }
}