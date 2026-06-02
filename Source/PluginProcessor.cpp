/*
    PluginProcessor.cpp — see header. Stubs only; DSP chain calls are wired but algorithm-free.
*/
#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "util/Parameters.h"

WonKnobberAudioProcessor::WonKnobberAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter (drive = new juce::AudioParameterFloat (
                      juce::ParameterID { ParamIDs::drive, 1 }, "Drive", 0.0f, 1.0f, 0.5f));
    addParameter (mix = new juce::AudioParameterFloat (
                      juce::ParameterID { ParamIDs::mix, 1 }, "Mix", 0.0f, 1.0f, 1.0f));
}

void WonKnobberAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const bool offline      = isNonRealtime();
    const int  osFactorLog2 = offline ? 5 : 4; // 32x offline polish / 16x realtime
    const int  numInputs    = juce::jmax (1, getTotalNumInputChannels());

    saturation.prepare (sampleRate, samplesPerBlock, numInputs, osFactorLog2, offline);
    setLatencySamples (saturation.getLatencySamples());

    convolution.prepare (sampleRate, samplesPerBlock);
    neuralModel.prepare (sampleRate, samplesPerBlock);

    // Size dry scratch defensively (prepare block can be smaller than later processBlock blocks in some hosts/offline).
    // 2048 is safe upper for most audio blocks; cheap prealloc.
    const int maxChans = juce::jmax (2, getTotalNumInputChannels());
    int safeBlock = juce::jmax (2048, samplesPerBlock);
    safeBlock = juce::jmax (safeBlock, 1);
    dryBuffer.setSize (maxChans, safeBlock);
    dryBuffer.clear();

    // Smooth mix at audio rate; 15 ms ramp (in 10-20 ms range) per spec to kill zippers.
    mixSmooth.reset (sampleRate, 0.015);

    dryWet.prepare (sampleRate, samplesPerBlock);
}

void WonKnobberAudioProcessor::releaseResources()
{
    saturation.reset();
    convolution.reset();
    neuralModel.reset();

    dryWet.reset();
    mixSmooth.setCurrentAndTargetValue (mixSmooth.getTargetValue());
}

bool WonKnobberAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

namespace
{
    // Lock-free max-merge: store newValue iff it's greater than the current value.
    // Audio thread only; GUI thread does the consume (exchange-with-zero) elsewhere.
    inline void atomicMaxMerge (std::atomic<float>& slot, float newValue) noexcept
    {
        float current = slot.load (std::memory_order_relaxed);
        while (newValue > current
               && ! slot.compare_exchange_weak (current, newValue,
                                                std::memory_order_relaxed,
                                                std::memory_order_relaxed))
        {
            // current was updated by compare_exchange to the latest value; loop.
        }
    }
}

void WonKnobberAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Input tap (pre-saturation). buffer.getMagnitude is absolute peak across the
    // requested range, which is exactly what a peak meter wants.
    const int  numCh   = buffer.getNumChannels();
    const int  numSmps = buffer.getNumSamples();
    if (numCh >= 1) atomicMaxMerge (peakInL, buffer.getMagnitude (0, 0, numSmps));
    if (numCh >= 2) atomicMaxMerge (peakInR, buffer.getMagnitude (1, 0, numSmps));

    saturation.setDrive (drive->get());

    // Update mix target each block (from param); smoothing happens inside applyCrossfade.
    if (mix != nullptr)
        mixSmooth.setTargetValue (mix->get());

    // Capture dry (pre-saturation) into scratch. Must be before any processing in the sat->conv chain.
    const int nch = buffer.getNumChannels();
    const int nsm = buffer.getNumSamples();
    const int nsmCopy = juce::jmin (nsm, dryBuffer.getNumSamples());
    for (int ch = 0; ch < nch && ch < dryBuffer.getNumChannels(); ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, nsmCopy);

    // Debug guard for block size edges (non-realtime/offline can exceed last prepare's block).
    jassert (nsm <= dryBuffer.getNumSamples());

    saturation.process (buffer);
    convolution.process (buffer);
    // neuralModel.process (buffer); // enabled once a model is loaded

    // Equal-power dry/wet after the full chain. Uses processor's mixSmooth (advances per sample).
    dryWet.applyCrossfade (mixSmooth, buffer, dryBuffer);

    // Output tap (post-chain, after mix for final I/O meter).
    if (numCh >= 1) atomicMaxMerge (peakOutL, buffer.getMagnitude (0, 0, numSmps));
    if (numCh >= 2) atomicMaxMerge (peakOutR, buffer.getMagnitude (1, 0, numSmps));
}

WonKnobberAudioProcessor::LevelSnapshot WonKnobberAudioProcessor::consumeMeterPeaks() noexcept
{
    LevelSnapshot s;
    s.inL  = peakInL .exchange (0.0f, std::memory_order_relaxed);
    s.inR  = peakInR .exchange (0.0f, std::memory_order_relaxed);
    s.outL = peakOutL.exchange (0.0f, std::memory_order_relaxed);
    s.outR = peakOutR.exchange (0.0f, std::memory_order_relaxed);
    return s;
}

juce::AudioProcessorEditor* WonKnobberAudioProcessor::createEditor()
{
    return new WonKnobberAudioProcessorEditor (*this);
}

void WonKnobberAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Layout: [float drive][float mix][String currentVariant]. Backwards-compatible.
    // Old float-only or drive+variant states will be handled in setStateInformation.
    juce::MemoryOutputStream stream (destData, true);
    stream.writeFloat (drive->get());
    stream.writeFloat (mix != nullptr ? mix->get() : 1.0f);
    stream.writeString (currentVariant);
}

void WonKnobberAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);
    if (stream.getNumBytesRemaining() >= (int) sizeof (float))
        *drive = stream.readFloat();

    if (stream.getNumBytesRemaining() >= (int) sizeof (float))
        *mix = stream.readFloat();
    else if (mix != nullptr)
        *mix = 1.0f; // backwards-compatible: old states (only drive float) default to full wet

    if (stream.getNumBytesRemaining() > 0)
    {
        const auto v = stream.readString();
        if (v.isNotEmpty())
            currentVariant = v;
    }

    // Extra trailing data (future versions) is ignored gracefully.

    // Sanitise both params after load (defensive for corrupt/NaN/Inf/out-of-range/truncated mix bytes).
    // AudioParameterFloat operator= does not auto-clamp; bad value here -> cos/sin(NaN) -> NaN audio in DryWet process.
    // Default 1.0 for mix preserves "old presets load with full wet preserved".
    auto sanitize = [] (juce::AudioParameterFloat* p, float def)
    {
        if (p == nullptr) return;
        float v = p->get();
        if (! std::isfinite (v)) v = def;
        v = juce::jlimit (0.0f, 1.0f, v);
        *p = v;
    };
    sanitize (drive, 0.5f);
    sanitize (mix, 1.0f);
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WonKnobberAudioProcessor();
}
