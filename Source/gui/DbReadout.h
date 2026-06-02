/*
    DbReadout.h — recessed amber-glow LED-style readout of the Drive value in dB.

    Spec (Claude Design / PHASE2B_DESIGN_SPEC.md §2):
      - Anchor: db_readout [421, 423, 161, 51]
      - Mapping: driveDB = drive * 48 - 24   (drive in [0,1] -> -24.0 ... +24.0 dB)
      - Display: leading "+" for >= 0, one decimal, " dB" suffix
        Examples: drive=0.5 -> "+0.0 dB"  drive=1.0 -> "+24.0 dB"  drive=0.0 -> "-24.0 dB"
      - Font: Orbitron tabular-nums (embedded via BinaryData)
      - Colour: #ff8800 with amber glow halo

    Notes:
      - Display mapping ONLY (Drive control's perceptual range). NOT the dBFS the
        I/O meters use — different scale, by design.
      - Same value also drives the drag value-bubble above the knob (Phase 2c
        polish — not in this PR).

    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class DbReadout : public juce::Component
{
public:
    DbReadout();

    void setDrive (float newDrive) noexcept;

    void paint (juce::Graphics& g) override;

private:
    float drive { 0.5f };
    juce::Typeface::Ptr orbitron;

    static constexpr float kSpanDB = 48.0f;     // total displayable range
    static constexpr float kOffsetDB = -24.0f;  // drive=0 maps to this
};
