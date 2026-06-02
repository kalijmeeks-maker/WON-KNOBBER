/*
    StatusLEDs.cpp — see header. Three top-strip dome LEDs (POWER · SIG · CLIP)
    driven off the same atomic peaks the I/O meter uses, with time-based hold/
    latch ballistics.
*/
#include "StatusLEDs.h"

#include <juce_audio_basics/juce_audio_basics.h>

void StatusLEDs::pushLevels (float inL, float inR, float outL, float outR, float dt) noexcept
{
    // SIG: tick down hold; re-arm if either input channel crosses -45 dBFS.
    sigHoldT = juce::jmax (0.0f, sigHoldT - dt);
    const float inPeak = juce::jmax (inL, inR);
    if (inPeak > 0.0f
        && juce::Decibels::gainToDecibels (inPeak, kSigThresholdDB - 1.0f) > kSigThresholdDB)
    {
        sigHoldT = kSigHoldSec;
    }

    // CLIP: tick down latch; latch on if either output channel crosses -0.1 dBFS.
    clipLatchT = juce::jmax (0.0f, clipLatchT - dt);
    const float outPeak = juce::jmax (outL, outR);
    if (outPeak > 0.0f
        && juce::Decibels::gainToDecibels (outPeak, kClipThresholdDB - 1.0f) >= kClipThresholdDB)
    {
        clipLatchT = kClipLatchSec;
    }

    repaint();
}

void StatusLEDs::drawDome (juce::Graphics& g, juce::Point<float> centre, float radius,
                           juce::Colour lit, juce::Colour glow, bool isOn) const
{
    const auto offColour = juce::Colour (0xff111111); // --mk-iron, LED off state

    // Outer soft glow (only when on) — radial bloom around the dome.
    if (isOn)
    {
        const float glowR = radius * 2.6f;
        juce::ColourGradient bloom (glow, centre, juce::Colour (0x00000000),
                                    centre.translated (glowR, 0.0f), true);
        bloom.addColour (0.55f, glow.withMultipliedAlpha (0.35f));
        g.setGradientFill (bloom);
        g.fillEllipse (centre.x - glowR, centre.y - glowR, glowR * 2.0f, glowR * 2.0f);
    }

    // Bezel ring — recessed metal frame around the dome.
    g.setColour (juce::Colour (0xff2a2a2c));
    g.fillEllipse (centre.x - radius - 1.5f, centre.y - radius - 1.5f,
                   (radius + 1.5f) * 2.0f, (radius + 1.5f) * 2.0f);
    g.setColour (juce::Colour (0xff0a0a0c));
    g.drawEllipse (centre.x - radius - 1.0f, centre.y - radius - 1.0f,
                   (radius + 1.0f) * 2.0f, (radius + 1.0f) * 2.0f, 0.8f);

    // Dome base fill — lit colour or off (iron).
    const auto base = isOn ? lit : offColour;
    juce::ColourGradient domeFill (base.brighter (isOn ? 0.20f : 0.0f),
                                   centre.translated (-radius * 0.35f, -radius * 0.35f),
                                   base.darker (0.55f),
                                   centre.translated (radius * 0.6f, radius * 0.6f),
                                   false);
    g.setGradientFill (domeFill);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Specular highlight — small bright dot top-left, gives the dome its glassy feel.
    if (isOn)
    {
        const float specR = radius * 0.30f;
        const auto specCentre = centre.translated (-radius * 0.30f, -radius * 0.32f);
        juce::ColourGradient spec (juce::Colours::white.withAlpha (0.85f), specCentre,
                                   juce::Colours::white.withAlpha (0.0f),
                                   specCentre.translated (specR, specR), true);
        g.setGradientFill (spec);
        g.fillEllipse (specCentre.x - specR, specCentre.y - specR,
                       specR * 2.0f, specR * 2.0f);
    }
}

void StatusLEDs::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty()) return;

    // The three LED centres are at 960x600 ref x=806,850,894 (all y=55). The component
    // is bound to a wide-but-short rect covering all three (left=POWER center -7,
    // right=CLIP center +7). We re-derive in-component fractions so it scales cleanly.
    // Reference span: leftmost 806 to rightmost 894 → 88 ref-px wide centre-to-centre.
    // Component width covers that span plus 7 px on either side (the dome radius).
    const float powerFracX = 0.0f / 88.0f;    // leftmost centre
    const float sigFracX   = 44.0f / 88.0f;   // middle centre
    const float clipFracX  = 88.0f / 88.0f;   // rightmost centre

    const float padX  = bounds.getWidth() * (7.0f / 102.0f); // 7-of-102 ref-px padding
    const float spanW = bounds.getWidth() - padX * 2.0f;
    const float yC    = bounds.getCentreY();

    const auto powerCentre = juce::Point<float> (bounds.getX() + padX + spanW * powerFracX, yC);
    const auto sigCentre   = juce::Point<float> (bounds.getX() + padX + spanW * sigFracX,   yC);
    const auto clipCentre  = juce::Point<float> (bounds.getX() + padX + spanW * clipFracX,  yC);

    const float radius = juce::jmin (bounds.getHeight() * 0.42f, padX); // 14-of-bounds-height

    // State resolution per spec: bypassed → all dark (POWER + SIG) and CLIP frozen.
    const bool powerOn = ! bypassed;
    const bool sigOn   = ! bypassed && sigHoldT > 0.0f;
    const bool clipOn  = clipLatchT > 0.0f; // CLIP stays lit through its latch even when bypassed

    drawDome (g, powerCentre, radius,
              juce::Colour (0xffffaa00),               // --mk-orange
              juce::Colour (0xb3ffaa00),               // --mk-amber-glow (~70% alpha)
              powerOn);
    drawDome (g, sigCentre, radius,
              juce::Colour (0xff4cff8e),               // --mk-mint
              juce::Colour (0x994cff8e),               // mint glow (~60% alpha)
              sigOn);
    drawDome (g, clipCentre, radius,
              juce::Colour (0xffff3300),               // --mk-red
              juce::Colour (0xb3ff3300),               // --mk-red-glow (~70% alpha)
              clipOn);
}
