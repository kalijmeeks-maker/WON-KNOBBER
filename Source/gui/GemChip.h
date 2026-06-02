/*
    GemChip.h — small persistent "current stone" pill below the hero knob.
    Click cycles to the next variant. Right-click (or popup-modifier click)
    pops the full 7-stone menu. Always visible — fits the single-knob ethos
    without a hidden drawer.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class GemChip : public juce::Component
{
public:
    GemChip()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setLabel (const juce::String& stone)
    {
        if (stone != currentLabel)
        {
            currentLabel = stone;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    // Editor sets these — chip stays UI-only and forwards user intent.
    std::function<void()>        onCycle;     // left-click
    std::function<void()>        onShowMenu;  // right-click / popup-modifier

private:
    juce::String currentLabel;
};
