/*
    IOMeter.h — Phase 2b horizontal twin-row peak meters (IN over OUT).

    Spec (Claude Design parity):
      - dB-scaled: floor -60 dBFS at fill 0.0, ceiling 0 dBFS at fill 1.0
      - Each row = single horizontal bar showing max(L,R)
      - IN row centred at 30% height, OUT row at 72% height
      - Fill gradient: 0.0 #1a9a48 → 0.6 #4cff8e → 0.78 #ff9a00 → 1.0 #ff3300
      - Peak-hold marker line: #ff5530 if held > 0.78, else #ffe0a0
      - Ballistics: instant rise; bar decay ~0.72/sec linear in fill;
                    peak-hold ~0.47s hold, then decay at 0.72/sec
      - Read-only audio tap: PluginProcessor::consumeMeterPeaks() — lock-free.

    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class IOMeter : public juce::Component
{
public:
    IOMeter() { setInterceptsMouseClicks (false, false); }

    // GUI-thread call from the editor's timer with linear magnitudes per channel.
    // dt is the seconds since the previous call — used to time-base the ballistics
    // so frame-rate jitter doesn't bias hold/decay.
    void pushPeaks (float inL, float inR, float outL, float outR, float dt) noexcept;

    // Bypass dim-state (§3.4): meters KEEP MOVING (audio still passes) but recolour neutral grey,
    // no peak-hold accent — separates "bypassed" from "crashed".
    void setBypassed (bool b) noexcept { bypassed = b; repaint(); }

    void paint (juce::Graphics& g) override;

private:
    bool bypassed { false };

    struct RowState
    {
        float bar     { 0.0f }; // current displayed bar fill (0..1)
        float holdPos { 0.0f }; // current peak-hold marker position (0..1)
        float holdT   { 0.0f }; // seconds remaining in hold phase
    };

    void advanceRow (RowState& s, float linearPeak, float dt) noexcept;

    RowState in {};
    RowState out {};

    static constexpr float kFloorDB   = -60.0f;
    static constexpr float kHoldSec   = 0.47f;
    static constexpr float kDecayRate = 0.72f; // per second, linear in fill space
};
