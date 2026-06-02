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
}

void WonKnobberAudioProcessor::releaseResources()
{
    saturation.reset();
    convolution.reset();
    neuralModel.reset();
}

bool WonKnobberAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

void WonKnobberAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    saturation.setDrive (drive->get());
    saturation.process (buffer);
    convolution.process (buffer);
    // neuralModel.process (buffer); // enabled once a model is loaded
}

juce::AudioProcessorEditor* WonKnobberAudioProcessor::createEditor()
{
    return new WonKnobberAudioProcessorEditor (*this);
}

void WonKnobberAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Layout: [float drive][String currentVariant]. Backwards-compatible with the
    // float-only state — older sets just won't have the variant byte after the float.
    juce::MemoryOutputStream stream (destData, true);
    stream.writeFloat (drive->get());
    stream.writeString (currentVariant);
}

void WonKnobberAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);
    if (stream.getNumBytesRemaining() >= (int) sizeof (float))
        *drive = stream.readFloat();
    if (stream.getNumBytesRemaining() > 0)
    {
        const auto v = stream.readString();
        if (v.isNotEmpty())
            currentVariant = v;
    }
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WonKnobberAudioProcessor();
}
