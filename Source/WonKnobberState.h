/*
    WonKnobberState.h — source-of-truth state snapshot for host state + presets (ValueTree round-trip)
    WON-KNOBBER · preset I/O foundation (no DSP, no GUI)
*/
#pragma once

#include <juce_data_structures/juce_data_structures.h>

struct WonKnobberState
{
    float drive{0.5f};
    float mix{1.0f};
    juce::String variant{"diamond"};
    bool bypass{false};
    // Cab (Convolution) + neural (RTNeural) slot selection, folded into each voice (schema v2).
    // Slot ids match the asset manifest. Defaults are deliberately OFF so legacy sessions (and any
    // state missing these attrs) load with identical audio to pre-cab/neural builds.
    juce::String cabIr{"FLAT"};    // FLAT/STUDIO_RIBBON/VINTAGE_4X12/CONSOLE_BOX/OLD_RADIO/IRON_CORE
    juce::String neuralModel{"NONE"}; // NONE/TAPE/VALVE/TRANSISTOR/IRON
    bool cabEngage{false};
    bool neuralEngage{false};
    // Future fields appended here only; NEVER reorder members (serialisation stability).

    juce::ValueTree toValueTree() const;
    static WonKnobberState fromValueTree(const juce::ValueTree& v);

    // Legacy v1.0.0 (pre-magic) positional reader. Fills this from old byte stream.
    static WonKnobberState fromLegacyV1Data(const void* data, int sizeInBytes);
};
