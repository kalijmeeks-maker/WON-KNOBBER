/*
    Convolution.cpp — see header. Embedded-IR cab stage built on juce::dsp::Convolution.
*/
#include "Convolution.h"

#include "BinaryData.h"

#include <cmath>
#include <iostream>

namespace
{
// Map a manifest cab IR slot id to its embedded WAV (BinaryData). Returns false for unknown ids.
bool irDataForId (const juce::String& id, const char*& data, int& size)
{
    if (id == "FLAT")          { data = BinaryData::ir_flat_wav;          size = BinaryData::ir_flat_wavSize;          return true; }
    if (id == "STUDIO_RIBBON") { data = BinaryData::ir_studio_ribbon_wav; size = BinaryData::ir_studio_ribbon_wavSize; return true; }
    if (id == "VINTAGE_4X12")  { data = BinaryData::ir_vintage_4x12_wav;  size = BinaryData::ir_vintage_4x12_wavSize;  return true; }
    if (id == "CONSOLE_BOX")   { data = BinaryData::ir_console_box_wav;    size = BinaryData::ir_console_box_wavSize;    return true; }
    if (id == "OLD_RADIO")     { data = BinaryData::ir_old_radio_wav;      size = BinaryData::ir_old_radio_wavSize;      return true; }
    if (id == "IRON_CORE")     { data = BinaryData::ir_iron_core_wav;      size = BinaryData::ir_iron_core_wavSize;      return true; }
    return false;
}
} // namespace

void Convolution::prepare (double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) blockSize;
    spec.numChannels = 2;
    convolution.prepare (spec); // a loaded IR is retained + resampled to this spec by JUCE
}

void Convolution::process (juce::AudioBuffer<float>& buffer)
{
    if (! engaged.load (std::memory_order_acquire))
        return; // cab off: bit-exact passthrough

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    convolution.process (ctx);
}

void Convolution::reset()
{
    convolution.reset();
}

void Convolution::setIr (const juce::String& cabIrId)
{
    if (cabIrId == currentIrId)
        return;

    const char* data = nullptr;
    int size = 0;
    if (! irDataForId (cabIrId, data, size))
        return; // unknown id: keep the current IR

    convolution.loadImpulseResponse (data, (size_t) size,
                                     juce::dsp::Convolution::Stereo::no,  // mono cab IR -> applied to all channels
                                     juce::dsp::Convolution::Trim::no,    // pre-trimmed in the asset pipeline
                                     0,                                   // use the whole IR
                                     juce::dsp::Convolution::Normalise::yes);
    currentIrId = cabIrId;
}

//==============================================================================
// Smoke tests (run at static init / dlopen, same pattern as WonKnobberState). Verify the
// engage gate is a bit-exact passthrough, that a valid embedded IR loads + processes finite,
// and that an unknown id is a safe no-op. Does NOT assert the async IR is fully swapped in
// (loadImpulseResponse finishes on a background thread).
namespace
{
static bool runConvolutionSmokeTests()
{
    bool pass = true;

    Convolution conv;
    conv.prepare (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buf.setSample (ch, i, std::sin ((float) i * 0.1f));

    juce::AudioBuffer<float> ref;
    ref.makeCopyOf (buf);

    // Disengaged -> bit-exact passthrough.
    conv.setEngaged (false);
    conv.process (buf);
    bool offOk = true;
    for (int ch = 0; ch < 2 && offOk; ++ch)
        for (int i = 0; i < 512; ++i)
            if (! juce::exactlyEqual (buf.getSample (ch, i), ref.getSample (ch, i)))
            {
                offOk = false;
                break;
            }
    std::cout << "CONVOLUTION [disengaged passthrough]: " << (offOk ? "PASS" : "FAIL") << std::endl;
    if (! offOk)
        pass = false;

    // Engaged with a valid embedded IR -> output stays finite (no crash, symbols resolve).
    conv.setIr ("FLAT");
    conv.setEngaged (true);
    conv.process (buf);
    bool finiteOk = true;
    for (int ch = 0; ch < 2 && finiteOk; ++ch)
        for (int i = 0; i < 512; ++i)
            if (! std::isfinite (buf.getSample (ch, i)))
            {
                finiteOk = false;
                break;
            }
    std::cout << "CONVOLUTION [engaged FLAT finite]: " << (finiteOk ? "PASS" : "FAIL") << std::endl;
    if (! finiteOk)
        pass = false;

    // Unknown id -> safe no-op.
    conv.setIr ("NOT_A_CAB");
    std::cout << "CONVOLUTION SMOKE TESTS OVERALL: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

static const bool convSmokeRan = runConvolutionSmokeTests();
} // namespace
