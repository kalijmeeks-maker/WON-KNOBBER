/*
    KnobLookAndFeel.h — LookAndFeel_V4 that renders the knob from a 120-frame filmstrip PNG.
    Hosts a variant map of all 7 gem filmstrips ("choose your stone") and an amber value-arc.
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel() = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    // Backwards-compatible single-strip setter (sets the "current" image directly).
    void setFilmstrip (const juce::Image& strip) { filmstrip = strip; }

    // Register a stone variant. Call once per stone at editor construction time;
    // the first registered variant is the default if none has been selected.
    void addVariant (const juce::String& stone, const juce::Image& strip);

    // Switch to a registered variant by name. No-op if the name isn't registered.
    // Repaints whatever slider is using this LookAndFeel via the host editor.
    void setVariant (const juce::String& stone);

    juce::String getCurrentVariant() const noexcept { return currentVariant; }

    // Read-only access to the registered variant names (insertion order).
    const juce::StringArray& getVariantNames() const noexcept { return variantNames; }

    // Bypass dim-state (§3.4): the value-arc fades to a dull grey-amber. Knob cap is hardware.
    void setBypassed (bool b) noexcept { bypassed = b; }

private:
    static constexpr int numFrames = 120; // Blender renders are 120-frame strips (256x30720)

    juce::Image filmstrip;             // the current variant's filmstrip
    juce::String currentVariant;       // stone name; empty until first addVariant
    juce::StringArray variantNames;    // insertion order — for the picker menu
    std::array<juce::Image, 16> variants{}; // backing store; index parallels variantNames
    bool bypassed { false };
};
