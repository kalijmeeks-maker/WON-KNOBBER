/*
    TransferModel.h — WON KNOBBER one-knob voicing map + memoryless transfer.
    Single source of truth shared by the DSP (Saturation.cpp) and the GUI scope
    (TransferCurve / HarmonicBars). The static transfer (Spiral2 presence with prev=x)
    matches won_density3_transfer.js and is what the scope draws — truthful to the audio's
    curve. (The real-time chain's 1-sample presence memory affects dynamics, not curve shape.)
*/
#pragma once

#include "AirwindowsShapers.h"

#include <cmath>

namespace wk
{
struct StageParams
{
    double density;      // Density3 amount (1.0 = neutral)
    double mojoMix;      // parallel Mojo blend (the "weight")
    double presence;     // Spiral2 presence blend (the "air")
    double purestInGain; // drive into the PurestSaturation ceiling
    double makeup;       // output trim to counter loudness bias
};

inline double clamp01 (double v) { return std::fmax (0.0, std::fmin (1.0, v)); }

inline double smoothstep (double e0, double e1, double x)
{
    if (e1 <= e0)
        return x < e0 ? 0.0 : 1.0;
    const double t = clamp01 ((x - e0) / (e1 - e0));
    return t * t * (3.0 - 2.0 * t);
}

// One-knob mapping: 0-20% Mojo glue · 20-50% Density3 toward fat · 50-80% thick + drive ·
// 80-100% Spiral2 presence + Purest ceiling assert harder.
inline StageParams computeStageParams (double d)
{
    StageParams p;
    p.density      = 1.0 + smoothstep (0.20, 1.0, d) * 3.0;
    p.mojoMix      = clamp01 (0.12 + 0.60 * (1.0 - smoothstep (0.0, 0.5, d)));
    p.presence     = smoothstep (0.80, 1.0, d) * 0.5;
    p.purestInGain = 1.0 + smoothstep (0.80, 1.0, d) * 0.8;
    p.makeup       = 1.0 - smoothstep (0.20, 1.0, d) * 0.15;
    return p;
}

// Memoryless wet transfer for the scope. drive,x in their usual ranges; returns y (wet).
inline double wonKnobberTransfer (double x, double drive)
{
    const StageParams p = computeStageParams (clamp01 (drive));
    double s = x;
    s = s * (1.0 - p.mojoMix) + aw::mojo (s) * p.mojoMix;
    s = aw::density3 (s, p.density);
    if (p.presence > 0.0)
        s = s * (1.0 - p.presence) + aw::spiralPresence (s, x) * p.presence;
    s = aw::purestSat (s, p.purestInGain, 1.0);
    s *= p.makeup;
    return s;
}
} // namespace wk
