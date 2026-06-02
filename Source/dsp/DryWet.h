/*
    DryWet.h — equal-power dry/wet crossfader for parallel saturation (real-time safe)
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

    // Apply equal-power crossfade using caller-provided (smoothed) mix value.
    // wetOut = dry * cos(m*π/2) + wet * sin(m*π/2)  [equal power]
    // Caller must advance the SmoothedValue per-sample inside; this just consumes via getNextValue.
    // RT-safe, no alloc/lock.
    void applyCrossfade (juce::SmoothedValue<float>& mix,
                         juce::AudioBuffer<float>& wetBuffer,
                         const juce::AudioBuffer<float>& dryBuffer);

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };
};
