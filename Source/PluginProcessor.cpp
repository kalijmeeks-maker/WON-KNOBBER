/*
    PluginProcessor.cpp — see header. Stubs only; DSP chain calls are wired but algorithm-free.
*/
#include "PluginProcessor.h"

#include "PluginEditor.h"
#include "util/Parameters.h"

WonKnobberAudioProcessor::WonKnobberAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(drive =
                     new juce::AudioParameterFloat(juce::ParameterID{ParamIDs::drive, 1}, "Drive", 0.0f, 1.0f, 0.5f));
    addParameter(mix = new juce::AudioParameterFloat(juce::ParameterID{ParamIDs::mix, 1}, "Mix", 0.0f, 1.0f, 1.0f));

    // Init A/B slots to the default current state (per foundation contract: slots = current until UI diverges).
    applyState(getCurrentState());
}

void WonKnobberAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const bool offline = isNonRealtime();
    const int osFactorLog2 = offline ? 5 : 4; // 32x offline polish / 16x realtime
    const int numInputs = juce::jmax(1, getTotalNumInputChannels());

    saturation.prepare(sampleRate, samplesPerBlock, numInputs, osFactorLog2, offline);
    setLatencySamples(saturation.getLatencySamples());

    convolution.prepare(sampleRate, samplesPerBlock);
    neuralModel.prepare(sampleRate, samplesPerBlock);

    // Size dry scratch defensively (prepare block can be smaller than later processBlock blocks in some hosts/offline).
    // 16384 covers offline render block sizes some hosts use (Logic/Reaper can push past 4k); cheap prealloc.
    const int maxChans = juce::jmax(2, getTotalNumInputChannels());
    int safeBlock = juce::jmax(16384, samplesPerBlock);
    safeBlock = juce::jmax(safeBlock, 1);
    dryBuffer.setSize(maxChans, safeBlock);
    dryBuffer.clear();

    // Smooth mix at audio rate; 15 ms ramp (in 10-20 ms range) per spec to kill zippers.
    mixSmooth.reset(sampleRate, 0.015);

    dryWet.prepare(sampleRate, samplesPerBlock);
}

void WonKnobberAudioProcessor::releaseResources()
{
    saturation.reset();
    convolution.reset();
    neuralModel.reset();

    dryWet.reset();
    mixSmooth.setCurrentAndTargetValue(mixSmooth.getTargetValue());
}

bool WonKnobberAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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
inline void atomicMaxMerge(std::atomic<float>& slot, float newValue) noexcept
{
    float current = slot.load(std::memory_order_relaxed);
    while (newValue > current &&
           !slot.compare_exchange_weak(current, newValue, std::memory_order_relaxed, std::memory_order_relaxed))
    {
        // current was updated by compare_exchange to the latest value; loop.
    }
}
} // namespace

void WonKnobberAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Input tap (pre-saturation). buffer.getMagnitude is absolute peak across the
    // requested range, which is exactly what a peak meter wants.
    const int numCh = buffer.getNumChannels();
    const int numSmps = buffer.getNumSamples();
    if (numCh >= 1)
        atomicMaxMerge(peakInL, buffer.getMagnitude(0, 0, numSmps));
    if (numCh >= 2)
        atomicMaxMerge(peakInR, buffer.getMagnitude(1, 0, numSmps));

    saturation.setDrive(drive->get());

    // Update mix target each block (from param); smoothing happens inside applyCrossfade.
    if (mix != nullptr)
        mixSmooth.setTargetValue(mix->get());

    // Capture dry (pre-saturation) into scratch. Must be before any processing in the sat->conv chain.
    const int nch = buffer.getNumChannels();
    const int nsm = buffer.getNumSamples();
    const int nsmCopy = juce::jmin(nsm, dryBuffer.getNumSamples());
    for (int ch = 0; ch < nch && ch < dryBuffer.getNumChannels(); ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, nsmCopy);

    // Debug guard for block size edges (non-realtime/offline can exceed last prepare's block).
    jassert(nsm <= dryBuffer.getNumSamples());

    saturation.process(buffer);
    convolution.process(buffer);
    // neuralModel.process (buffer); // enabled once a model is loaded

    // Equal-power dry/wet after the full chain. Uses processor's mixSmooth (advances per sample).
    dryWet.applyCrossfade(mixSmooth, buffer, dryBuffer);

    // Output tap (post-chain, after mix for final I/O meter).
    if (numCh >= 1)
        atomicMaxMerge(peakOutL, buffer.getMagnitude(0, 0, numSmps));
    if (numCh >= 2)
        atomicMaxMerge(peakOutR, buffer.getMagnitude(1, 0, numSmps));
}

WonKnobberAudioProcessor::LevelSnapshot WonKnobberAudioProcessor::consumeMeterPeaks() noexcept
{
    LevelSnapshot s;
    s.inL = peakInL.exchange(0.0f, std::memory_order_relaxed);
    s.inR = peakInR.exchange(0.0f, std::memory_order_relaxed);
    s.outL = peakOutL.exchange(0.0f, std::memory_order_relaxed);
    s.outR = peakOutR.exchange(0.0f, std::memory_order_relaxed);
    return s;
}

WonKnobberState WonKnobberAudioProcessor::getCurrentState() const noexcept
{
    WonKnobberState s;
    s.drive = drive ? drive->get() : 0.5f;
    s.mix = mix ? mix->get() : 1.0f;
    s.variant = currentVariant;
    s.bypass = bypassState;
    return s;
}

void WonKnobberAudioProcessor::applyState(const WonKnobberState& st) noexcept
{
    if (drive != nullptr)
        *drive = st.drive;
    currentVariant = st.variant;
    if (mix != nullptr)
        *mix = st.mix;
    bypassState = st.bypass;

    slotA = st;
    slotB = st;
}

juce::AudioProcessorEditor* WonKnobberAudioProcessor::createEditor()
{
    return new WonKnobberAudioProcessorEditor(*this);
}

void WonKnobberAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // NEW layout (v1+): magic "WK2\0" (4 bytes) + XML (ValueTree.createXml()->toString() bytes).
    // The root VT is "WONKNOBBER" (version=1) with:
    //  - child[0]: WonKnobberState for the live/current snapshot (no "slot" prop)
    //  - child[1]: WonKnobberState for slotA (with slot="A")
    //  - child[2]: WonKnobberState for slotB (with slot="B")
    // This ensures A/B round-trips via host state even before UI (per contract).
    // Old pre-magic positional states fall back to legacy reader (zero data loss).
    //
    // Legacy v1.0.0 positional (pre this PR): [float drive][String variant][float mix].
    // Mix appended after variant so #27 (drive+variant only) states default mix to 1.0 cleanly.
    // All old cases (drive-only, drive+variant, +mix) load with bypass=false, mix=1.0 where missing.

    WonKnobberState curr = getCurrentState();

    juce::MemoryOutputStream stream(destData, true);
    stream.write("WK2", 3);
    stream.writeByte(0);

    juce::ValueTree rootVT{"WONKNOBBER"};
    rootVT.setProperty("version", 1, nullptr);

    rootVT.addChild(curr.toValueTree(), -1, nullptr);

    auto sa = slotA.toValueTree();
    sa.setProperty("slot", "A", nullptr);
    rootVT.addChild(sa, -1, nullptr);

    auto sb = slotB.toValueTree();
    sb.setProperty("slot", "B", nullptr);
    rootVT.addChild(sb, -1, nullptr);

    auto xml = rootVT.createXml();
    const juce::String xmlText = xml->toString();
    stream.write(xmlText.toRawUTF8(), xmlText.getNumBytesAsUTF8());
}

void WonKnobberAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, (size_t)sizeInBytes, false);

    // Peek magic header (4 bytes: 'W','K','2','\0')
    char header[4] = {};
    if (stream.getNumBytesRemaining() >= 4)
        stream.read(header, 4);

    const bool hasMagic = (header[0] == 'W' && header[1] == 'K' && header[2] == '2' && header[3] == '\0');

    if (hasMagic)
    {
        // New path: remaining bytes are the UTF-8 XML text for the WONKNOBBER root VT.
        juce::String xmlText;
        if (stream.getNumBytesRemaining() > 0)
        {
            juce::MemoryBlock mb;
            stream.readIntoMemoryBlock(mb);
            xmlText = juce::String::fromUTF8(static_cast<const char*>(mb.getData()), static_cast<int>(mb.getSize()));
        }

        if (auto xml = juce::XmlDocument::parse(xmlText))
        {
            auto vt = juce::ValueTree::fromXml(*xml);
            WonKnobberState loaded = WonKnobberState::fromValueTree(vt);

            applyState(loaded);

            // Restore explicit slot children if present (overriding the init-to-current for A/B roundtrip).
            if (vt.isValid())
            {
                for (int i = 0; i < vt.getNumChildren(); ++i)
                {
                    auto c = vt.getChild(i);
                    if (c.getType() == juce::Identifier("WonKnobberState") && c.hasProperty("slot"))
                    {
                        const juce::String sid = c.getProperty("slot").toString();
                        WonKnobberState ss = WonKnobberState::fromValueTree(c);
                        if (sid == "A")
                            slotA = ss;
                        else if (sid == "B")
                            slotB = ss;
                    }
                }
            }
        }
        // If parse fails, fall through to defaults (sanitised below via fromValueTree path).
    }
    else
    {
        // Legacy positional fallback (pre-magic states from v1.0.0 and earlier PRs).
        // Delegates to WonKnobberState::fromLegacyV1Data which replicates the guarded reads + defaults + sanitize.
        WonKnobberState leg = WonKnobberState::fromLegacyV1Data(data, sizeInBytes);
        applyState(leg);
    }

    // Extra trailing data ignored. All paths above already sanitised (isfinite + jlimit).
    // Old states always yield bypass=false, mix=1.0 where absent, variant preserved, drive ok.
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WonKnobberAudioProcessor();
}
