/*
    DryWet.cpp — see header.
    Equal-power crossfade (dry * cos(mix*π/2) + wet * sin(mix*π/2)) with smoothed param.
*/
#include "DryWet.h"

#include <cmath>

void DryWet::prepare (double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    blockSize  = newBlockSize > 0 ? newBlockSize : 512;
    // No internal state to reset beyond rates; smoothing lives with caller (processor).
}

void DryWet::process (juce::AudioBuffer<float>& buffer)
{
    // Contract requires process(); the crossfade logic is in applyCrossfade (takes external SmoothedValue).
    juce::ignoreUnused (buffer);
}

void DryWet::reset()
{
    // Nothing; smoothing reset is caller's responsibility (in processor).
}

void DryWet::applyCrossfade (juce::SmoothedValue<float>& mix,
                             juce::AudioBuffer<float>& wetBuffer,
                             const juce::AudioBuffer<float>& dryBuffer)
{
    const int numCh = juce::jmin (wetBuffer.getNumChannels(), dryBuffer.getNumChannels());
    const int numS  = juce::jmin (wetBuffer.getNumSamples(),  dryBuffer.getNumSamples());

    if (numCh <= 0)
        return;

    for (int ch = 0; ch < numCh; ++ch)
    {
        float*       wet = wetBuffer.getWritePointer (ch);
        const float* dry = dryBuffer.getReadPointer (ch);

        const int thisNumS = juce::jmin (numS, dryBuffer.getNumSamples()); // defensive
        for (int n = 0; n < thisNumS; ++n)
        {
            const float m = mix.getNextValue();
            const float c = std::cos (m * juce::MathConstants<float>::halfPi);
            const float s = std::sin (m * juce::MathConstants<float>::halfPi);
            wet[n] = dry[n] * c + wet[n] * s;
        }

        // Handle tail overrun (wet nsm > dry prepared size): use last dry (or 0) + continue smoothing to avoid unblended full-wet glitch on tail when mix != 1.
        const int wetN = wetBuffer.getNumSamples();
        if (wetN > thisNumS)
        {
            const int lastIdx = juce::jmax (0, thisNumS - 1);
            const float lastDry = (thisNumS > 0 ? dry[lastIdx] : 0.0f);
            for (int n = thisNumS; n < wetN; ++n)
            {
                const float m = mix.getNextValue();
                const float c = std::cos (m * juce::MathConstants<float>::halfPi);
                const float s = std::sin (m * juce::MathConstants<float>::halfPi);
                wet[n] = lastDry * c + wet[n] * s;
            }
        }
    }

    // Debug guard (triggers in dbg builds on overrun edges; handled above for release safety).
    jassert (wetBuffer.getNumSamples() <= dryBuffer.getNumSamples());
}
