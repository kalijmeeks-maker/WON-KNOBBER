/*
    HarmonicBars.h — harmonic spectrum scope. Drives a sine through the real transfer
    (wk::wonKnobberTransfer) and shows DFT magnitudes of harmonics 1..7 at the current Drive.
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

class HarmonicBars : public juce::Component
{
public:
    HarmonicBars() { setInterceptsMouseClicks (false, false); }

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
