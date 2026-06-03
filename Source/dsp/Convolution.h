/*
    Convolution.h — wraps juce::dsp::Convolution for IR-based cab coloration
    WON-KNOBBER · part of the dsp layer
*/
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>

class Convolution
{
public:
    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer);
    void reset();

    // Engage/disengage the cab stage. Audio-thread-safe: process() is a bit-exact passthrough
    // when off, so the cab adds nothing until a voice (or the rear panel) turns it on.
    void setEngaged (bool shouldEngage) noexcept { engaged.store (shouldEngage, std::memory_order_release); }

    // Select the cab IR by manifest slot id (FLAT/STUDIO_RIBBON/VINTAGE_4X12/CONSOLE_BOX/OLD_RADIO/IRON_CORE).
    // Loads the embedded WAV from BinaryData; juce::dsp::Convolution does the resample + thread-safe
    // swap on its own background thread, so this is safe to call from the message thread on preset load.
    // Unknown ids are ignored (current IR stays). Call on the message thread only.
    void setIr (const juce::String& cabIrId);

private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };

    // Zero-latency head so preset-driven IR swaps never change reported PDC (per integration spec).
    juce::dsp::Convolution convolution { juce::dsp::Convolution::Latency { 0 } };

    std::atomic<bool> engaged { false };
    juce::String currentIrId; // message-thread only; dedupes redundant reloads
};
