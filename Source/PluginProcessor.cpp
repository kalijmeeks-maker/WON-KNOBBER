/*
    PluginProcessor.cpp — see header. Stubs only; DSP chain calls are wired but algorithm-free.
*/
#include "PluginProcessor.h"

#include "BinaryData.h"
#include "PluginEditor.h"
#include "util/Parameters.h"

#include <cmath>
#include <iostream>

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
    convolution.setEngaged(cabEngage);
    if (cabEngage)
        convolution.setIr(currentCabIr); // re-apply IR for the (possibly new) sample rate
    neuralModel.prepare(sampleRate, samplesPerBlock);
    neuralModel.setEngaged(neuralEngage);
    if (neuralEngage)
        neuralModel.setModel(currentNeuralModel);

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
    neuralModel.process(buffer); // gated internally: passthrough unless neuralEngage + a model is loaded

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
    s.cabIr = currentCabIr;
    s.neuralModel = currentNeuralModel;
    s.cabEngage = cabEngage;
    s.neuralEngage = neuralEngage;
    return s;
}

void WonKnobberAudioProcessor::applyStateToParams(const WonKnobberState& st) noexcept
{
    if (drive != nullptr)
        *drive = st.drive;
    currentVariant = st.variant;
    if (mix != nullptr)
        *mix = st.mix;
    bypassState = st.bypass;
    currentCabIr = st.cabIr;
    currentNeuralModel = st.neuralModel;
    cabEngage = st.cabEngage;
    neuralEngage = st.neuralEngage;

    // Reflect cab selection into the convolution stage (message thread). The engage gate is atomic
    // and the IR load is queued wait-free by JUCE. Neural wiring lands in a later PR.
    convolution.setEngaged(cabEngage);
    if (cabEngage)
        convolution.setIr(currentCabIr);

    // Reflect neural selection into the RTNeural stage (message thread). Engage gate is atomic;
    // the model is built + atomically swapped in by NeuralModel. Off by default until a voice engages it.
    neuralModel.setEngaged(neuralEngage);
    if (neuralEngage)
        neuralModel.setModel(currentNeuralModel);
}

void WonKnobberAudioProcessor::applyState(const WonKnobberState& st) noexcept
{
    applyStateToParams(st);
    slotA = st;
    slotB = st;
    activeSlot = 'A';
    // Seat the "loaded voice" baseline so isDirty() reads false right after any full apply
    // (ctor init, factory load, host recall). Manual param edits after this flip it true.
    loadedVoice = st;
}

bool WonKnobberAudioProcessor::isDirty() const
{
    // Design call (modified-dot spec): only the four hidden cab/neural identity fields count as
    // "modified" — NOT drive/mix. Riding DRIVE/MIX is the expected one-knob interaction and is
    // already visible on the face; the dot exists to surface a rear override you can't otherwise see.
    const WonKnobberState live = getCurrentState();
    if (live.cabIr != loadedVoice.cabIr)
        return true;
    if (live.neuralModel != loadedVoice.neuralModel)
        return true;
    if (live.cabEngage != loadedVoice.cabEngage)
        return true;
    if (live.neuralEngage != loadedVoice.neuralEngage)
        return true;
    return false;
}

void WonKnobberAudioProcessor::revertToLoadedPreset()
{
    // Design call: revert ONLY the four identity fields (cab/neural) to the loaded voice; leave
    // drive/mix/variant where the user has them. Clears the dirty flag without touching the knobs.
    WonKnobberState s = getCurrentState();
    s.cabIr = loadedVoice.cabIr;
    s.neuralModel = loadedVoice.neuralModel;
    s.cabEngage = loadedVoice.cabEngage;
    s.neuralEngage = loadedVoice.neuralEngage;
    applyStateToParams(s);
}

namespace
{
// The 8 factory voices (Claude Design's drive/mix/gem map). Display name + embedded XML run
// in the same order; the gem variant lives in each XML so loading a preset changes the hero
// stone (intentional per-voice identity).
struct FactoryPreset
{
    const char* name;
    const char* xml;
    int xmlSize;
};
const FactoryPreset kFactoryPresets[] = {
    { "TAPE HEAD", BinaryData::tape_head_xml, BinaryData::tape_head_xmlSize },
    { "CONSOLE GLUE", BinaryData::console_glue_xml, BinaryData::console_glue_xmlSize },
    { "FURNACE", BinaryData::furnace_xml, BinaryData::furnace_xmlSize },
    { "VELVET", BinaryData::velvet_xml, BinaryData::velvet_xmlSize },
    { "SUNDAY DRIVE", BinaryData::sunday_drive_xml, BinaryData::sunday_drive_xmlSize },
    { "TUBE WARM", BinaryData::tube_warm_xml, BinaryData::tube_warm_xmlSize },
    { "DIODE BITE", BinaryData::diode_bite_xml, BinaryData::diode_bite_xmlSize },
    { "TRANSFORMER", BinaryData::transformer_xml, BinaryData::transformer_xmlSize },
};
} // namespace

int WonKnobberAudioProcessor::getNumFactoryPresets() const
{
    return (int) juce::numElementsInArray(kFactoryPresets);
}

juce::String WonKnobberAudioProcessor::getFactoryPresetName(int i) const
{
    if (i < 0 || i >= getNumFactoryPresets())
        return {};
    return kFactoryPresets[i].name;
}

void WonKnobberAudioProcessor::loadFactoryPreset(int i)
{
    if (i < 0 || i >= getNumFactoryPresets())
        return;

    const auto xmlText = juce::String::fromUTF8(kFactoryPresets[i].xml, kFactoryPresets[i].xmlSize);
    if (auto xml = juce::XmlDocument::parse(xmlText))
    {
        auto vt = juce::ValueTree::fromXml(*xml);
        WonKnobberState st = WonKnobberState::fromValueTree(vt);
        applyState(st);
    }
}

char WonKnobberAudioProcessor::getActiveSlot() const
{
    return activeSlot;
}

void WonKnobberAudioProcessor::setActiveSlot(char which)
{
    which = static_cast<char>(juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(which)));
    if (which != 'A' && which != 'B')
        return;
    if (which == activeSlot)
        return;

    // Save outgoing live values to the outgoing slot (compare semantics: do not lose tweaks on switch).
    WonKnobberState curr = getCurrentState();
    if (activeSlot == 'A')
        slotA = curr;
    else
        slotB = curr;

    // Apply target slot to live params (no clobber of slots).
    const WonKnobberState& target = (which == 'A' ? slotA : slotB);
    applyStateToParams(target);
    activeSlot = which;
}

void WonKnobberAudioProcessor::copySlot(char src, char dst)
{
    src = static_cast<char>(juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(src)));
    dst = static_cast<char>(juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(dst)));
    if (src != 'A' && src != 'B')
        return;
    if (dst != 'A' && dst != 'B')
        return;
    if (src == dst)
        return;

    if (src == 'A')
        slotB = slotA;
    else
        slotA = slotB;
}

void WonKnobberAudioProcessor::saveToActiveSlot()
{
    WonKnobberState curr = getCurrentState();
    if (activeSlot == 'A')
        slotA = curr;
    else
        slotB = curr;
}

void WonKnobberAudioProcessor::loadFromActiveSlot()
{
    const WonKnobberState& s = (activeSlot == 'A' ? slotA : slotB);
    applyStateToParams(s);
}

void WonKnobberAudioProcessor::undoLast()
{
    // Revert active by switching to the other slot without saving current live (true undo of last switch/tweak).
    char other = (activeSlot == 'A' ? 'B' : 'A');
    const WonKnobberState& target = (other == 'A' ? slotA : slotB);
    applyStateToParams(target);
    activeSlot = other;
}

void WonKnobberAudioProcessor::randomizeParameters()
{
    // UI thread only (message thread). Not called from audio.
    auto& rng = juce::Random::getSystemRandom();
    if (drive != nullptr)
        *drive = rng.nextFloat();
    if (mix != nullptr)
        *mix = rng.nextFloat();

    static const juce::StringArray variants{"diamond", "onyx", "sapphire", "emerald", "ruby", "amethyst", "citrine"};
    currentVariant = variants[rng.nextInt(variants.size())];
    // bypass left as-is
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

            activeSlot = 'A';
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

    activeSlot = 'A';

    // Extra trailing data ignored. All paths above already sanitised (isfinite + jlimit).
    // Old states always yield bypass=false, mix=1.0 where absent, variant preserved, drive ok.
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WonKnobberAudioProcessor();
}

//==============================================================================
// Self-tests for Phase 2b preset strip + transport API (run at static init / dlopen of plugin).
// Mirrors the style + visibility of WonKnobberState unit tests. Exercises factory load, A/B
// save-on-switch, copy, transport save/load/undo/randomize. Output on stdout for build/qa logs.
namespace
{
static bool runPresetTransportAPITests()
{
    bool pass = true;

    WonKnobberAudioProcessor proc;

    // num + names
    {
        int n = proc.getNumFactoryPresets();
        bool nOk = (n == 8);
        std::cout << "PRESET API [numFactory]: " << (nOk ? "PASS" : "FAIL") << " n=" << n << std::endl;
        if (!nOk)
            pass = false;

        juce::String n0 = proc.getFactoryPresetName(0);
        juce::String n1 = proc.getFactoryPresetName(1);
        bool namesOk = (n0 == "TAPE HEAD" && n1 == "CONSOLE GLUE");
        std::cout << "PRESET API [names]: " << (namesOk ? "PASS" : "FAIL") << " 0=" << n0 << " 1=" << n1 << std::endl;
        if (!namesOk)
            pass = false;
    }

    // load factory (FURNACE = index 2, drive~0.86 variant=ruby)
    {
        proc.loadFactoryPreset(2);
        float ld = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;
        auto lv = proc.getCurrentVariant();
        bool loadOk = (std::abs(ld - 0.86f) < 0.01f && lv == "ruby");
        std::cout << "PRESET API [load FURNACE]: " << (loadOk ? "PASS" : "FAIL") << " drive=" << ld << " var=" << lv
                  << std::endl;
        if (!loadOk)
            pass = false;
    }

    // active init after load
    {
        char act = proc.getActiveSlot();
        bool actOk = (act == 'A');
        std::cout << "PRESET API [active after factory]: " << (actOk ? "PASS" : "FAIL") << " act=" << act << std::endl;
        if (!actOk)
            pass = false;
    }

    // A/B switch saves outgoing, applies target
    {
        // from FURNACE on A, tweak live, switch to B (saves tweak to A, live gets B's ~0.86)
        if (auto* d = proc.getDriveParameter())
            *d = 0.42f;
        proc.setActiveSlot('B');
        float liveD = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;
        bool toBOK = (proc.getActiveSlot() == 'B' && std::abs(liveD - 0.86f) < 0.01f);
        // now switch back to A: should get the saved 0.42
        proc.setActiveSlot('A');
        liveD = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;
        bool savedOK = (std::abs(liveD - 0.42f) < 0.01f && proc.getActiveSlot() == 'A');
        bool abOK = toBOK && savedOK;
        std::cout << "PRESET API [AB switch+save]: " << (abOK ? "PASS" : "FAIL") << std::endl;
        if (!abOK)
            pass = false;
    }

    // copySlot
    {
        proc.copySlot('A', 'B');
        proc.setActiveSlot('B');
        float liveD = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;
        bool cOK = (std::abs(liveD - 0.42f) < 0.01f);
        std::cout << "PRESET API [copySlot]: " << (cOK ? "PASS" : "FAIL") << std::endl;
        if (!cOK)
            pass = false;
    }

    // transport: save to active, randomize, loadFrom restores the saved
    {
        if (auto* d = proc.getDriveParameter())
            *d = 0.99f;
        proc.saveToActiveSlot();
        float savedDrive = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;

        proc.randomizeParameters();
        float postR = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;
        bool rOK = (std::abs(postR - savedDrive) > 0.001f); // almost always
        std::cout << "PRESET API [randomize]: " << (rOK ? "PASS" : "FAIL") << " pre=" << savedDrive << " post=" << postR
                  << std::endl;
        if (!rOK)
            pass = false;

        proc.loadFromActiveSlot();
        float postL = proc.getDriveParameter() ? proc.getDriveParameter()->get() : 0.0f;
        bool lOK = (std::abs(postL - savedDrive) < 0.001f);
        std::cout << "PRESET API [save/load slot]: " << (lOK ? "PASS" : "FAIL") << std::endl;
        if (!lOK)
            pass = false;
    }

    // undoLast
    {
        char before = proc.getActiveSlot();
        proc.undoLast();
        char after = proc.getActiveSlot();
        bool uOK = (after != before);
        std::cout << "PRESET API [undoLast]: " << (uOK ? "PASS" : "FAIL") << " " << before << "->" << after
                  << std::endl;
        if (!uOK)
            pass = false;
    }

    // isDirty / revertToLoadedPreset: only the four cab/neural identity fields count as "modified"
    // (Design call — NOT drive/mix). Loading a voice seats the baseline (clean), and riding DRIVE/MIX
    // must stay CLEAN. Divergence of the four fields -> dirty is driven by the rear-panel override
    // (PR 4); until those setters land the four fields only change via a preset load, which re-stamps.
    {
        proc.loadFactoryPreset(0); // TAPE HEAD
        bool cleanAfterLoad = !proc.isDirty();

        if (auto* d = proc.getDriveParameter())
            *d = 0.42f + 0.2f; // ride DRIVE — must stay CLEAN under the four-field rule
        bool cleanAfterDrive = !proc.isDirty();

        proc.loadFactoryPreset(2);   // FURNACE: different cab/neural — re-stamps, still clean
        bool cleanAfterReload = !proc.isDirty();
        proc.revertToLoadedPreset(); // no-op while clean
        bool cleanAfterRevert = !proc.isDirty();

        bool dirtyOK = cleanAfterLoad && cleanAfterDrive && cleanAfterReload && cleanAfterRevert;
        std::cout << "PRESET API [isDirty four-field]: " << (dirtyOK ? "PASS" : "FAIL")
                  << " load=" << (cleanAfterLoad ? 1 : 0) << " drive-clean=" << (cleanAfterDrive ? 1 : 0)
                  << " reload=" << (cleanAfterReload ? 1 : 0) << " revert=" << (cleanAfterRevert ? 1 : 0) << std::endl;
        if (!dirtyOK)
            pass = false;
    }

    std::cout << "PRESET TRANSPORT API TESTS OVERALL: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

static const bool presetAPITestsRan = runPresetTransportAPITests();
} // namespace
