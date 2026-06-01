/*
    KnobLookAndFeel.h — LookAndFeel_V4 that renders the knob from a 120-frame filmstrip PNG
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel() = default;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void setFilmstrip (const juce::Image& strip) { filmstrip = strip; }

private:
    static constexpr int numFrames = 120; // Blender renders are 120-frame strips (256x30720)
    juce::Image filmstrip;
};
