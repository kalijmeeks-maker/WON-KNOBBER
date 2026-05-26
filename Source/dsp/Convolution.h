/*
    Convolution.h — wraps juce::dsp::Convolution for IR-based coloration
    WON-KNOBBER · part of the dsp layer
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class Convolution
{
public:
    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    // Loads a WAV IR (mono/stereo, 48 kHz reference) on the message thread.
    void loadIR (const juce::File& irFile);

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };
    juce::dsp::Convolution convolution;
};
