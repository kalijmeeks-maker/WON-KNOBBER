/*
    IOMeter.cpp — see header. Twin horizontal IN/OUT peak meters, dB-scaled,
    with time-based peak-hold ballistics. Spec sourced from Claude Design.
*/
#include "IOMeter.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
    constexpr float kFloorDB = -60.0f;

    // Linear magnitude → bar fill (0..1) in dB space: -60 dBFS → 0, 0 dBFS → 1.
    inline float linearToFill (float lin) noexcept
    {
        if (lin <= 0.0f) return 0.0f;
        const float db = juce::Decibels::gainToDecibels (lin, kFloorDB);
        return juce::jlimit (0.0f, 1.0f, (db - kFloorDB) / -kFloorDB);
    }
}

void IOMeter::advanceRow (RowState& s, float linearPeak, float dt) noexcept
{
    const float target = linearToFill (linearPeak);

    // Bar: instant rise, time-based linear decay.
    if (target > s.bar)
        s.bar = target;
    else
        s.bar = juce::jmax (0.0f, s.bar - kDecayRate * dt);

    // Peak-hold marker: latches the highest fill seen, holds, then decays.
    if (target > s.holdPos)
    {
        s.holdPos = target;
        s.holdT   = kHoldSec;
    }
    else if (s.holdT > 0.0f)
    {
        s.holdT = juce::jmax (0.0f, s.holdT - dt);
    }
    else
    {
        s.holdPos = juce::jmax (0.0f, s.holdPos - kDecayRate * dt);
    }
}

void IOMeter::pushPeaks (float inL, float inR, float outL, float outR, float dt) noexcept
{
    // One bar per row showing the louder channel — keeps the bar thick enough to
    // read inside the 73-px-tall anchor while still surfacing the true peak.
    advanceRow (in,  juce::jmax (inL,  inR),  dt);
    advanceRow (out, juce::jmax (outL, outR), dt);
    repaint();
}

void IOMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty()) return;

    // Track + bar geometry. Spec: IN row centred @30% height, OUT @72%.
    const float trackThickness = juce::jmax (3.0f, bounds.getHeight() * 0.18f);
    const float padX           = bounds.getHeight() * 0.05f;
    const float trackLeft      = bounds.getX() + padX;
    const float trackRight     = bounds.getRight() - padX;
    const float trackW         = trackRight - trackLeft;

    auto drawRow = [&] (const RowState& s, float yCentreFrac)
    {
        const float yC = bounds.getY() + bounds.getHeight() * yCentreFrac;
        const juce::Rectangle<float> track (trackLeft,
                                            yC - trackThickness * 0.5f,
                                            trackW,
                                            trackThickness);

        // Recessed track background.
        g.setColour (juce::Colour (0xff0a0a0c).withAlpha (0.92f));
        g.fillRoundedRectangle (track, trackThickness * 0.35f);

        // Filled portion in the Design gradient (left → right).
        if (s.bar > 0.0f)
        {
            juce::ColourGradient grad (juce::Colour (0xff1a9a48), track.getX(),     0.0f,
                                       juce::Colour (0xffff3300), track.getRight(), 0.0f, false);
            grad.addColour (0.60f, juce::Colour (0xff4cff8e));
            grad.addColour (0.78f, juce::Colour (0xffff9a00));
            g.setGradientFill (grad);

            const auto fill = track.withWidth (track.getWidth() * s.bar);
            // Clip the rounded mask so the gradient terminates cleanly inside the track.
            juce::Graphics::ScopedSaveState save (g);
            juce::Path mask;
            mask.addRoundedRectangle (track, trackThickness * 0.35f);
            g.reduceClipRegion (mask);
            g.fillRect (fill);
        }

        // Peak-hold marker (1.5-px vertical line). Colour shifts at the amber knee.
        if (s.holdPos > 0.0f)
        {
            const float markerX = track.getX() + track.getWidth() * s.holdPos;
            g.setColour (s.holdPos > 0.78f ? juce::Colour (0xffff5530)
                                            : juce::Colour (0xffffe0a0));
            g.fillRect (juce::Rectangle<float> (markerX - 0.75f,
                                                track.getY() - 1.0f,
                                                1.5f,
                                                track.getHeight() + 2.0f));
        }

        // Hairline edge so the track reads against the dark brushed-metal strip.
        g.setColour (juce::Colour (0xff000000).withAlpha (0.55f));
        g.drawRoundedRectangle (track, trackThickness * 0.35f, 0.6f);
    };

    drawRow (in,  0.30f);
    drawRow (out, 0.72f);
}
