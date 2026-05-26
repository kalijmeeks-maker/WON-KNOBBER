/*
    Saturation.cpp — see header. Stub: no DSP math yet.
*/
#include "Saturation.h"

void Saturation::prepare (double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;
    drive.reset (sampleRate, 0.02); // 20 ms smoothing
}

void Saturation::process (juce::AudioBuffer<float>& buffer)
{
    juce::ignoreUnused (buffer);
    // TODO: implement tanh soft-clip using the smoothed drive value.
    // Real-time safe: no allocation / locks / I/O here.
}

void Saturation::reset()
{
    drive.setCurrentAndTargetValue (drive.getTargetValue());
}

void Saturation::setDrive (float newDrive)
{
    drive.setTargetValue (juce::jlimit (0.0f, 1.0f, newDrive));
}
