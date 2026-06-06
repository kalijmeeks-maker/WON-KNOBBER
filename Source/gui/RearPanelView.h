/*
    RearPanelView.h — the flip-to-rear "service panel" (960x612). Draws the photoreal rose/amber
    rear chassis (baked PNG) and paints the LIVE layers over it per docs/rear-panel-anchors.json (#51):
    CABINET + NEURAL engage rockers/LEDs + IR/model selection wells, plus the flip-to-front medallion.
    Static labels/recesses (module frames, oversampling seg, I/O-trim strip, power entry) are baked
    into the PNG. Cab/neural selection is the rear-fold "override": it drives the processor's cab/neural
    state directly, which lights the front modified-from-preset dot when it diverges from the loaded voice.

    NOTE: the flip is wired — clicking the medallion (full hero-well hit-target) runs the front<->rear
    rotation via FlipTransition. Oversampling, I/O-trim, and ABOUT/MANUAL are baked-only placeholders
    with no click handlers yet (PR4 scope). SHIP-REQUIRED before 1.0: wire rear ABOUT to the licences
    modal (MIT/Airwindows notice — ABOUT is its only rear entry point).
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class RearPanelView : public juce::Component
{
public:
    RearPanelView();

    void paint(juce::Graphics& g) override;
    // About/Licences modal is drawn ABOVE the rear children (incl. the bypass wash) so it dims the
    // rear face — the front modal lives in FaceplateView, which is hidden while the rear is flipped up.
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override; // Esc steps Licences->About->close

    // State push from the editor (machine ids: cab FLAT.., neural NONE..). No-ops if unchanged.
    void setCabState(const juce::String& cabIrId, bool engaged);
    void setNeuralState(const juce::String& neuralModelId, bool engaged);
    void setBypassed(bool b);

    // Open the About/Licences modal programmatically. Used by the headless render harness to snapshot
    // the modal over the rear face; the same state is normally driven by the baked ABOUT link click.
    void openAboutModal(bool showLicences = false);

    // Callbacks to the processor (message thread).
    std::function<void(bool)> onCabEngageToggled;
    std::function<void(juce::String)> onCabIrChanged;
    std::function<void(bool)> onNeuralEngageToggled;
    std::function<void(juce::String)> onNeuralModelChanged;
    std::function<void()> onFlipToFront;

private:
    static constexpr int kRefW = 960;
    static constexpr int kRefH = 612;

    juce::Image rearPlate;

    // Live state mirrored from the processor (machine ids).
    juce::String cabIrId{"FLAT"};
    juce::String neuralModelId{"NONE"};
    bool cabEngaged{false};
    bool neuralEngaged{false};
    bool bypassed{false};

    // Anchors (960x612 ref) recomputed to actual bounds in resized().
    juce::Rectangle<int> cabEngageRockerBounds, cabEngageLedBounds, cabIrWellBounds;
    juce::Rectangle<int> neuralEngageRockerBounds, neuralEngageLedBounds, neuralModelWellBounds;
    juce::Rectangle<int> flipMedallionBounds;
    juce::Rectangle<int> aboutLinkBounds; // baked "ABOUT" text link at the right end of the io_trim_strip
    // Chevron sub-rects inside each well (prev/next steppers).
    juce::Rectangle<int> cabPrevBounds, cabNextBounds, neuralPrevBounds, neuralNextBounds;

    void drawEngage(juce::Graphics& g, juce::Rectangle<int> rocker, juce::Rectangle<int> led, bool on) const;
    void drawWell(juce::Graphics& g, juce::Rectangle<int> well, juce::Rectangle<int> prev, juce::Rectangle<int> next,
                  const juce::String& display) const;

    // About/Licences modal (self-contained on the rear; opened by the baked ABOUT link, dismissed by
    // its close box / scrim / Esc). Mirrors the front modal's geometry + behaviour; legal text is
    // single-sourced from wk::about. Drawn in paintOverChildren so the scrim dims the rear face.
    bool aboutVisible{false};
    bool licencesVisible{false}; // full licence scroll, layered above the About card
    int licencesScrollY{0};      // current vertical scroll offset (px, >= 0)
    int licencesMaxScrollY{0};   // clamp bound recomputed each paint from text height
    void drawAboutPanel(juce::Graphics& g);
    void drawLicencesPanel(juce::Graphics& g);
    juce::Rectangle<int> computeAboutPanelBounds() const;    // centred modal rect (shared by paint + hit-test)
    juce::Rectangle<int> computeLicencesLinkBounds() const;  // "View full licences ▸" hit-target in the About card
    juce::Rectangle<int> computeLicencesPanelBounds() const; // centred licences scroll rect

    // Ordered option lists + id<->display maps (mirror docs/cab-neural-id-bridge.json).
    static const juce::StringArray& cabIds();
    static const juce::StringArray& neuralIds();
    static juce::String cabDisplay(const juce::String& id);
    static juce::String neuralDisplay(const juce::String& id);
    static juce::String cycle(const juce::StringArray& ids, const juce::String& current, int dir);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RearPanelView)
};
