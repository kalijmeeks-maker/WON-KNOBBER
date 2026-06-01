/*
    Saturation.h — WON KNOBBER one-knob saturation engine (real-time safe)

    Signal chain (per sample, inside an oversampled region):
        Mojo glue  ->  Density3 hero  ->  Spiral2 presence  ->  PurestSaturation ceiling

    One "Drive" knob (0..1) crossfades the stage intensities. Oversampling factor is
    chosen by the host context: 16x realtime (IIR, low latency) / 32x offline (FIR,
    linear-phase, max quality). Saturation algorithms are MIT-licensed Airwindows
    ports — see AirwindowsShapers.h.
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <memory>

class Saturation
{
public:
    // oversampleFactorLog2: oversampling ratio = 2^factor (4 -> 16x, 5 -> 32x).
    // offlineHighQuality: use linear-phase FIR (offline) vs low-latency IIR (realtime).
    void prepare (double sampleRate, int blockSize, int numChannels,
                  int oversampleFactorLog2, bool offlineHighQuality);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    void setDrive (float newDrive); // 0.0 - 1.0

    int getLatencySamples() const;  // oversampling latency, at base sample rate

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::SmoothedValue<float> drive { 0.5f };
    double prevSample[2] { 0.0, 0.0 }; // Spiral2 presence state, per channel (L/R)
};
