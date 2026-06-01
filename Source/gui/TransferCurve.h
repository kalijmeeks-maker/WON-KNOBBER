/*
    TransferCurve.h — live transfer-curve scope. Draws the real WON KNOBBER transfer
    (wk::wonKnobberTransfer) at the current Drive, over the chassis TRANSFER panel.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

class TransferCurve : public juce::Component
{
public:
    TransferCurve() { setInterceptsMouseClicks (false, false); }

    void setDrive (float newDrive)
    {
        if (std::abs (newDrive - drive) > 1.0e-4f)
        {
            drive = newDrive;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override;

private:
    float drive { 0.5f };
};
