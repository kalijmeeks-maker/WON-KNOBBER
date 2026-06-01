/*
    Saturation.cpp — see header.

    WON KNOBBER DSP identity:
        Density3 is the engine.  Mojo is the weight.  Spiral2 is the air.
        PurestSaturation is the guardrail.  Oversampling is the polish.
*/
#include "Saturation.h"
#include "AirwindowsShapers.h"

#include <cmath>

namespace
{
// Per-sample stage intensities derived from the single "Drive" knob (0..1).
// These curves are the voicing — tuned by ear, safe to re-shape later.
struct StageParams
{
    double density;      // Density3 amount (1.0 = neutral)
    double mojoMix;      // parallel Mojo blend (the "weight")
    double presence;     // Spiral2 presence blend (the "air")
    double purestInGain; // drive into the PurestSaturation ceiling
    double makeup;       // output trim to counter loudness bias
};

inline double smoothstep (double e0, double e1, double x)
{
    if (e1 <= e0)
        return x < e0 ? 0.0 : 1.0;
    const double t = juce::jlimit (0.0, 1.0, (x - e0) / (e1 - e0));
    return t * t * (3.0 - 2.0 * t);
}

// One-knob mapping (see WON KNOBBER spec):
//   0-20%   clean thickening, mostly Mojo
//   20-50%  Density3 toward neutral/fat
//   50-80%  Density3 thick saturation, more drive
//   80-100% Spiral2 presence + Purest ceiling assert harder
inline StageParams computeStageParams (double d)
{
    StageParams p;
    p.density      = 1.0 + smoothstep (0.20, 1.0, d) * 3.0;                 // 1.0 -> 4.0
    p.mojoMix      = juce::jlimit (0.0, 1.0, 0.12 + 0.60 * (1.0 - smoothstep (0.0, 0.5, d)));
    p.presence     = smoothstep (0.80, 1.0, d) * 0.5;
    p.purestInGain = 1.0 + smoothstep (0.80, 1.0, d) * 0.8;
    p.makeup       = 1.0 - smoothstep (0.20, 1.0, d) * 0.15;
    return p;
}
} // namespace

void Saturation::prepare (double sampleRate, int blockSize, int numChannels,
                          int oversampleFactorLog2, bool offlineHighQuality)
{
    const int channels = juce::jmax (1, numChannels);
    const int ratio    = 1 << oversampleFactorLog2;

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) channels,
        (size_t) oversampleFactorLog2,
        offlineHighQuality ? juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple
                           : juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,                  // maximum quality
        offlineHighQuality);   // integer latency for clean offline alignment

    oversampler->initProcessing ((size_t) blockSize);
    oversampler->reset();

    // Smooth at the oversampled rate so getNextValue() advances per OS sample.
    drive.reset (sampleRate * (double) ratio, 0.02); // 20 ms

    for (auto& s : prevSample)
        s = 0.0;
}

void Saturation::process (juce::AudioBuffer<float>& buffer)
{
    if (oversampler == nullptr)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    auto osBlock = oversampler->processSamplesUp (block);

    const size_t numSamples  = osBlock.getNumSamples();
    const size_t numChannels = osBlock.getNumChannels();

    for (size_t n = 0; n < numSamples; ++n)
    {
        const double d = (double) drive.getNextValue();
        const StageParams p = computeStageParams (d);

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            float* data = osBlock.getChannelPointer (ch);
            const double x = (double) data[n];

            double s = x;
            s = s * (1.0 - p.mojoMix) + aw::mojo (s) * p.mojoMix;   // weight
            s = aw::density3 (s, p.density);                        // engine

            if (p.presence > 0.0 && ch < 2)
                s = s * (1.0 - p.presence) + aw::spiralPresence (s, prevSample[ch]) * p.presence; // air

            if (ch < 2)
                prevSample[ch] = x;

            s = aw::purestSat (s, p.purestInGain, 1.0);             // guardrail
            s *= p.makeup;

            data[n] = (float) s;
        }
    }

    oversampler->processSamplesDown (block);
}

void Saturation::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();

    for (auto& s : prevSample)
        s = 0.0;

    drive.setCurrentAndTargetValue (drive.getTargetValue());
}

void Saturation::setDrive (float newDrive)
{
    drive.setTargetValue (juce::jlimit (0.0f, 1.0f, newDrive));
}

int Saturation::getLatencySamples() const
{
    return oversampler != nullptr ? (int) std::lround (oversampler->getLatencyInSamples()) : 0;
}
