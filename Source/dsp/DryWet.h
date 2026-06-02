/*
    DryWet.h — equal-power dry/wet crossfader (real-time safe)
    WON-KNOBBER · part of the dsp layer
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class DryWet
{
public:
    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    // Apply the crossfade using caller-provided smoothed mix (0 = full dry, 1 = full wet).
    // dryBuffer must be pre-filled with the pre-effect audio.
    void applyCrossfade (juce::SmoothedValue<float>& mix,
                         juce::AudioBuffer<float>& wetBuffer,
                         const juce::AudioBuffer<float>& dryBuffer);

private:
    // TODO: implement fields if needed
};