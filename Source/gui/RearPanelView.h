/*
    RearPanelView.h — the flip-to-rear "service panel" (960x600). Draws the photoreal rose/amber
    rear chassis (baked PNG) and paints the LIVE layers over it per docs/rear-panel-anchors.json (#51):
    CABINET + NEURAL engage rockers/LEDs + IR/model selection wells, plus the flip-to-front medallion.
    Static labels/recesses (module frames, oversampling seg, I/O-trim strip, power entry) are baked
    into the PNG. Cab/neural selection is the rear-fold "override": it drives the processor's cab/neural
    state directly, which lights the front modified-from-preset dot when it diverges from the loaded voice.

    NOTE (PR4 scope): the flip AFFORDANCE visual + front<->rear rotation transition are deferred until
    Design's flip-spec.html lands; the medallion here is a plain click-to-return hit-target for now.
    Oversampling + I/O-trim are baked-only placeholders (no params yet).
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
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // State push from the editor (machine ids: cab FLAT.., neural NONE..). No-ops if unchanged.
    void setCabState(const juce::String& cabIrId, bool engaged);
    void setNeuralState(const juce::String& neuralModelId, bool engaged);
    void setBypassed(bool b);

    // Callbacks to the processor (message thread).
    std::function<void(bool)> onCabEngageToggled;
    std::function<void(juce::String)> onCabIrChanged;
    std::function<void(bool)> onNeuralEngageToggled;
    std::function<void(juce::String)> onNeuralModelChanged;
    std::function<void()> onFlipToFront;

private:
    static constexpr int kRefW = 960;
    static constexpr int kRefH = 600;

    juce::Image rearPlate;

    // Live state mirrored from the processor (machine ids).
    juce::String cabIrId{"FLAT"};
    juce::String neuralModelId{"NONE"};
    bool cabEngaged{false};
    bool neuralEngaged{false};
    bool bypassed{false};

    // Anchors (960x600 ref) recomputed to actual bounds in resized().
    juce::Rectangle<int> cabEngageRockerBounds, cabEngageLedBounds, cabIrWellBounds;
    juce::Rectangle<int> neuralEngageRockerBounds, neuralEngageLedBounds, neuralModelWellBounds;
    juce::Rectangle<int> flipMedallionBounds;
    // Chevron sub-rects inside each well (prev/next steppers).
    juce::Rectangle<int> cabPrevBounds, cabNextBounds, neuralPrevBounds, neuralNextBounds;

    void drawEngage(juce::Graphics& g, juce::Rectangle<int> rocker, juce::Rectangle<int> led, bool on) const;
    void drawWell(juce::Graphics& g, juce::Rectangle<int> well, juce::Rectangle<int> prev, juce::Rectangle<int> next,
                  const juce::String& display) const;

    // Ordered option lists + id<->display maps (mirror docs/cab-neural-id-bridge.json).
    static const juce::StringArray& cabIds();
    static const juce::StringArray& neuralIds();
    static juce::String cabDisplay(const juce::String& id);
    static juce::String neuralDisplay(const juce::String& id);
    static juce::String cycle(const juce::StringArray& ids, const juce::String& current, int dir);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RearPanelView)
};
