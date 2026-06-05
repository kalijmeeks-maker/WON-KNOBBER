/*
    TransferCurve.cpp — see header. Transparent over the chassis TRANSFER panel:
    faint grid + unity diagonal + amber transfer curve from wk::wonKnobberTransfer.
*/
#include "TransferCurve.h"

#include "../dsp/TransferModel.h"

namespace
{
    // Self-drawn recessed well — same machined "seated" recipe as the footer bay, so the
    // scope reads as cut into the brushed plate instead of painted onto a flat faceplate.
    void drawRecessedWell (juce::Graphics& g, juce::Rectangle<float> well)
    {
        constexpr float cornerR = 9.0f;

        // dark interior: vertical gradient, darker at the top lip, lifting toward the floor
        juce::ColourGradient floor (juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.44f),
                                    well.getX(), well.getY(),
                                    juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.20f),
                                    well.getX(), well.getBottom(), false);
        g.setGradientFill (floor);
        g.fillRoundedRectangle (well, cornerR);

        // soft inset top-shadow band — light falloff from the upper lip into the interior
        const float bandH = well.getHeight() * 0.28f;
        juce::ColourGradient topShade (juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.55f),
                                       well.getX(), well.getY(),
                                       juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.0f),
                                       well.getX(), well.getY() + bandH, false);
        g.setGradientFill (topShade);
        g.fillRoundedRectangle (well.withHeight (bandH), cornerR);

        // 1px inner stroke — seats the cut edge
        g.setColour (juce::Colour::fromFloatRGBA (0.0f, 0.0f, 0.0f, 0.62f));
        g.drawRoundedRectangle (well, cornerR, 1.0f);

        // 1px top lip highlight — the machined chamfer catching light
        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, 0.05f));
        g.drawHorizontalLine ((int) well.getY(), well.getX() + cornerR, well.getRight() - cornerR);
    }
}

void TransferCurve::paint (juce::Graphics& g)
{
    drawRecessedWell (g, getLocalBounds().toFloat().reduced (1.0f));

    const auto b = getLocalBounds().toFloat().reduced (6.0f);

    // faint grid
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (int i = 1; i < 4; ++i)
    {
        const float fx = b.getX() + b.getWidth()  * (float) i / 4.0f;
        const float fy = b.getY() + b.getHeight() * (float) i / 4.0f;
        g.drawVerticalLine   ((int) fx, b.getY(), b.getBottom());
        g.drawHorizontalLine ((int) fy, b.getX(), b.getRight());
    }

    // unity reference diagonal
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (b.getX(), b.getBottom(), b.getRight(), b.getY(), 1.0f);

    // the transfer curve: x in [-1,1] -> y in [-1,1]
    juce::Path path;
    constexpr int N = 160;
    for (int i = 0; i <= N; ++i)
    {
        const double x = -1.0 + 2.0 * (double) i / (double) N;
        const double y = wk::wonKnobberTransfer (x, (double) drive);
        const float px = b.getX() + (float) ((x + 1.0) * 0.5) * b.getWidth();
        const float py = b.getBottom() - (float) ((y + 1.0) * 0.5) * b.getHeight();
        if (i == 0) path.startNewSubPath (px, py);
        else        path.lineTo (px, py);
    }
    g.setColour (juce::Colour (0xffFE9A00)); // brand amber
    g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
}
