/*
    Saturation.h — tanh soft-clip drive stage (real-time safe)
    WON-KNOBBER · part of the dsp layer
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class Saturation
{
public:
    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    void setDrive (float newDrive); // 0.0 - 1.0

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };
    juce::SmoothedValue<float> drive { 0.5f };
};
