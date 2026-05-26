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
    saturation.prepare (sampleRate, samplesPerBlock);
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
    juce::MemoryOutputStream stream (destData, true);
    stream.writeFloat (drive->get());
}

void WonKnobberAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);
    if (stream.getNumBytesRemaining() >= (int) sizeof (float))
        *drive = stream.readFloat();
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WonKnobberAudioProcessor();
}
