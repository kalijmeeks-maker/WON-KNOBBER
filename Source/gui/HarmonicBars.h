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
        if (bypassed) return; // §3: frozen on the last frame while bypassed (no live motion)
        if (std::abs (newDrive - drive) > 1.0e-4f)
        {
            drive = newDrive;
            repaint();
        }
    }

    // Bypass dim-state (tokens §3.4): drop to 25% opacity + freeze.
    void setBypassed (bool b)
    {
        if (bypassed == b) return;
        bypassed = b;
        setAlpha (b ? 0.25f : 1.0f);
    }

    void paint (juce::Graphics& g) override;

private:
    float drive { 0.5f };
    bool bypassed { false };
};
