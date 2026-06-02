/*
    StatusLEDs.h — POWER · SIG · CLIP top-strip LEDs.

    Spec (Claude Design / PHASE2B_DESIGN_SPEC.md §1):
      - POWER  (806,55) amber  #ffaa00  — engaged state (not bypassed). Steady.
      - SIG    (850,55) mint   #4cff8e  — input > -45 dBFS, ~150 ms hold so it
                                          doesn't flicker on transients-only material.
      - CLIP   (894,55) red    #ff3300  — post-chain peak >= -0.1 dBFS, latches on
                                          ~1.0 s, then clears.
      - LED off: #111111 (chassis iron). Each dome is 14px in 960x600 ref space.

    Drives off the same atomic peak buffers as the I/O meter — no new audio-thread
    work. Time-based hold/latch ballistics from a real dt so frame-rate jitter
    doesn't bias them.

    Bypass: when wired (separate PR), POWER + SIG go dark and CLIP freezes. For now
    POWER is hard-on (engaged = true) until the bypass param lands.

    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class StatusLEDs : public juce::Component
{
public:
    StatusLEDs() { setInterceptsMouseClicks (false, false); }

    // GUI-thread push from the editor timer with linear magnitudes per channel
    // (same shape as the I/O meter's pushPeaks). dt is seconds since last push.
    void pushLevels (float inL, float inR, float outL, float outR, float dt) noexcept;

    // Bypass state, mirrors the bypass_rocker once that lands. For now editor
    // leaves this at the default (false / engaged).
    void setBypassed (bool b) noexcept { bypassed = b; repaint(); }

    void paint (juce::Graphics& g) override;

private:
    void drawDome (juce::Graphics& g, juce::Point<float> centre, float radius,
                   juce::Colour lit, juce::Colour glow, bool isOn) const;

    bool  bypassed     { false };
    float sigHoldT     { 0.0f };  // seconds remaining of SIG visible hold
    float clipLatchT   { 0.0f };  // seconds remaining of CLIP latch

    static constexpr float kSigThresholdDB  = -45.0f;
    static constexpr float kClipThresholdDB =  -0.1f;
    static constexpr float kSigHoldSec      =  0.15f;
    static constexpr float kClipLatchSec    =  1.0f;
};
