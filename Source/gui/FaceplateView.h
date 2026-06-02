/*
    FaceplateView.h — root container: draws the photoreal PRO chassis (960x600 PNG)
    and seats the hero filmstrip knob in the SATURATION well.
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include "BypassLED.h"
#include "DbReadout.h"
#include "GemChip.h"
#include "HarmonicBars.h"
#include "IOMeter.h"
#include "KnobLookAndFeel.h"
#include "MixKnob.h"
#include "StatusLEDs.h"
#include "TransferCurve.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class FaceplateView : public juce::Component
{
public:
    FaceplateView();
    ~FaceplateView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    juce::Slider& getDriveKnob() { return driveKnob; }
    juce::Slider& getMixKnob() { return mixKnob.getSlider(); }
    BypassLED& getBypassLED() { return bypassLed; }

    // Push the current Drive (0..1) to the live scopes (transfer curve + harmonics)
    // AND the dB readout. Single source-of-truth so the well and the scopes can never
    // drift relative to the actual parameter.
    void setDrive (float d)
    {
        transferCurve.setDrive (d);
        harmonicBars.setDrive (d);
        dbReadout.setDrive (d);
    }

    // Push per-block linear peaks (pre + post chain) into the I/O meter at the
    // editor's timer rate. dt = seconds since the previous push, used for the
    // meter's time-based hold/decay ballistics.
    void pushLevels(float inL, float inR, float outL, float outR, float dt) noexcept
    {
        ioMeter.pushPeaks(inL, inR, outL, outR, dt);
        statusLEDs.pushLevels(inL, inR, outL, outR, dt);
    }

    // "Choose your stone" — switch the hero filmstrip to a registered variant
    // (diamond/onyx/sapphire/emerald/ruby/amethyst/citrine). Repaints knob + chip.
    void setVariant(const juce::String& stone)
    {
        knobLnf.setVariant(stone);
        gemChip.setLabel(stone);
        driveKnob.repaint();
    }
    juce::String getVariant() const { return knobLnf.getCurrentVariant(); }
    const juce::StringArray& getVariantNames() const { return knobLnf.getVariantNames(); }

    // Editor wires this — chip click cycles to the next stone and notifies back
    // so the processor can persist the choice. String arg = newly selected stone.
    std::function<void(const juce::String&)> onVariantPicked;

    // Phase 2b preset strip + transport (footer). Editor wires these to processor
    // preset/slot/transport methods. Callbacks fire on user gesture; editor syncs live state + scopes.
    std::function<void(int)> onFactoryPresetSelected;           // arg = 0..N-1 factory index to load
    std::function<void(char)> onActiveSlotSelected;             // 'A' or 'B'
    std::function<void(const juce::String&)> onTransportAction; // "save", "load", "undo", "randomize"

    // Called by editor after preset load / slot switch / state recall to keep display in sync.
    void setPresetDisplayName(const juce::String& name);
    void setActiveSlot(char s); // 'A'/'B'
    char getActiveSlot() const { return activeSlot; }

private:
    // Design reference is the 960x600 PRO chassis; controls are placed in its coords.
    static constexpr int kRefW = 960;
    static constexpr int kRefH = 600;

    KnobLookAndFeel knobLnf;
    juce::Slider driveKnob;
    MixKnob mixKnob; // DRY/WET mix; reuses KnobLookAndFeel without filmstrip (ellipse fallback, always circular)
    TransferCurve transferCurve; // live transfer-curve scope (TRANSFER panel)
    HarmonicBars harmonicBars;   // harmonic spectrum scope (HARMONICS panel)
    GemChip gemChip;             // persistent "choose your stone" pill below the knob
    IOMeter    ioMeter;    // Phase 2b: twin IN/OUT peak meters (io_meters anchor)
    StatusLEDs statusLEDs; // Phase 2b: POWER / SIG / CLIP dome LEDs (top strip)
    DbReadout  dbReadout;  // Phase 2b: drive value -> dB readout (db_readout anchor)
    BypassLED  bypassLed;  // Phase 2b: live bypass telltale (not shown yet)

    juce::Image faceplate; // embedded photoreal chassis (BinaryData)

    // Phase 2b footer preset strip + transport (placed via anchors; drawn + hit-tested inline, no extra child comps).
    juce::String currentPresetName{"Default"};
    char activeSlot{'A'};
    int currentPresetIndex{0};

    // Cached scaled rects (from place() in resized) for drawing the overlays and mouse hit-testing in footer.
    juce::Rectangle<int> presetStripBounds, transportBounds;
    juce::Rectangle<int> chevLeftBounds, chevRightBounds, aButtonBounds, bButtonBounds;
    juce::Rectangle<int> presetNameBounds; // the engraved LED-style display area inside strip
    juce::Rectangle<int> saveBtnBounds, loadBtnBounds, undoBtnBounds, randBtnBounds;

    void drawPresetStrip(juce::Graphics& g);
    void drawTransportTray(juce::Graphics& g);
    void cyclePreset(int dir); // -1 prev, +1 next; wraps; fires callback
};
