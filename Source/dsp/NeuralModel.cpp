/*
    NeuralModel.cpp — see header. RTNeural feedforward inference, RT-safe, off by default.
    Models are 2-in / 2-out (stereo) RTNeural json_parser exports (dense + tanh).
*/
#include "NeuralModel.h"

#include "BinaryData.h"

#include <RTNeural/RTNeural.h>

#include <atomic>
#include <cmath>
#include <iostream>
#include <sstream>

struct NeuralModel::Impl
{
    using ModelT = RTNeural::Model<float>;

    std::atomic<bool> engaged { false };
    std::unique_ptr<ModelT> models[2];      // double-buffered: audio reads active, message thread builds inactive
    std::atomic<int> activeIdx { 0 };
    juce::String currentModelId;            // message-thread only; dedupes redundant reloads
};

namespace
{
bool modelDataForId (const juce::String& id, const char*& data, int& size)
{
    if (id == "TAPE")       { data = BinaryData::model_tape_json;       size = BinaryData::model_tape_jsonSize;       return true; }
    if (id == "VALVE")      { data = BinaryData::model_valve_json;      size = BinaryData::model_valve_jsonSize;      return true; }
    if (id == "TRANSISTOR") { data = BinaryData::model_transistor_json; size = BinaryData::model_transistor_jsonSize; return true; }
    if (id == "IRON")       { data = BinaryData::model_iron_json;       size = BinaryData::model_iron_jsonSize;       return true; }
    return false; // NONE / unknown -> no model
}
} // namespace

NeuralModel::NeuralModel() : impl (std::make_unique<Impl>()) {}
NeuralModel::~NeuralModel() = default;

void NeuralModel::prepare (double, int) {} // feedforward dense/tanh: sample-rate agnostic, no scratch to size

void NeuralModel::setEngaged (bool shouldEngage) noexcept
{
    impl->engaged.store (shouldEngage, std::memory_order_release);
}

void NeuralModel::process (juce::AudioBuffer<float>& buffer)
{
    if (! impl->engaged.load (std::memory_order_acquire))
        return; // off: bit-exact passthrough

    auto* model = impl->models[impl->activeIdx.load (std::memory_order_acquire)].get();
    if (model == nullptr)
        return; // no model loaded: passthrough

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int numSmps = buffer.getNumSamples();
    float in[2] = { 0.0f, 0.0f };
    for (int i = 0; i < numSmps; ++i)
    {
        in[0] = buffer.getSample (0, i);
        in[1] = numCh > 1 ? buffer.getSample (1, i) : in[0];
        model->forward (in); // returns out[0]; full stereo output via getOutputs()
        const float* out = model->getOutputs();
        buffer.setSample (0, i, out[0]);
        if (numCh > 1)
            buffer.setSample (1, i, out[1]);
    }
}

void NeuralModel::reset()
{
    for (auto& m : impl->models)
        if (m != nullptr)
            m->reset();
}

void NeuralModel::setModel (const juce::String& modelId)
{
    if (modelId == impl->currentModelId)
        return;

    const int inactive = 1 - impl->activeIdx.load (std::memory_order_acquire);

    const char* data = nullptr;
    int size = 0;
    if (! modelDataForId (modelId, data, size))
    {
        impl->models[inactive].reset(); // NONE/unknown -> passthrough slot
        impl->activeIdx.store (inactive, std::memory_order_release);
        impl->currentModelId = modelId;
        return;
    }

    std::unique_ptr<Impl::ModelT> built;
    try
    {
        const std::string jsonText (data, (size_t) size);
        auto j = nlohmann::json::parse (jsonText);
        built = RTNeural::json_parser::parseJson<float> (j);
    }
    catch (...)
    {
        built.reset(); // graceful: bad/unsupported JSON -> no model (passthrough)
    }

    // Build into the inactive slot, then flip the active index (release). The audio thread only
    // ever reads the active slot and re-reads activeIdx each block, so it never touches a slot the
    // message thread is mutating. Preset-driven swaps are seconds apart — well outside one block.
    impl->models[inactive] = std::move (built);
    impl->activeIdx.store (inactive, std::memory_order_release);
    impl->currentModelId = modelId;
}

//==============================================================================
// Smoke tests (run at static init / dlopen, same pattern as Convolution/WonKnobberState):
// disengaged passthrough is bit-exact, an engaged real model stays finite, NONE is a safe no-op.
namespace
{
static bool runNeuralModelSmokeTests()
{
    bool pass = true;

    NeuralModel nm;
    nm.prepare (48000.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buf.setSample (ch, i, 0.25f * std::sin ((float) i * 0.1f));

    juce::AudioBuffer<float> ref;
    ref.makeCopyOf (buf);

    // Disengaged -> bit-exact passthrough.
    nm.setEngaged (false);
    nm.process (buf);
    bool offOk = true;
    for (int ch = 0; ch < 2 && offOk; ++ch)
        for (int i = 0; i < 512; ++i)
            if (! juce::exactlyEqual (buf.getSample (ch, i), ref.getSample (ch, i)))
            {
                offOk = false;
                break;
            }
    std::cout << "NEURAL [disengaged passthrough]: " << (offOk ? "PASS" : "FAIL") << std::endl;
    if (! offOk)
        pass = false;

    // Engaged with a real embedded model -> output stays finite (model loads + runs).
    nm.setModel ("TAPE");
    nm.setEngaged (true);
    nm.process (buf);
    bool finiteOk = true;
    for (int ch = 0; ch < 2 && finiteOk; ++ch)
        for (int i = 0; i < 512; ++i)
            if (! std::isfinite (buf.getSample (ch, i)))
            {
                finiteOk = false;
                break;
            }
    std::cout << "NEURAL [engaged TAPE finite]: " << (finiteOk ? "PASS" : "FAIL") << std::endl;
    if (! finiteOk)
        pass = false;

    nm.setModel ("NONE"); // safe no-op
    std::cout << "NEURAL SMOKE TESTS OVERALL: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

static const bool neuralSmokeRan = runNeuralModelSmokeTests();
} // namespace
