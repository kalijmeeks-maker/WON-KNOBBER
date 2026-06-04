/*
    HarmonicBars.cpp — see header. DFT of a sine driven through the active transfer;
    bars 1..7 normalised to the fundamental, drawn over the chassis HARMONICS panel.
*/
#include "HarmonicBars.h"

#include "../dsp/TransferModel.h"

#include <cmath>

namespace
{
    // Self-drawn recessed well — same machined "seated" recipe as the footer bay, so the
    // bars read as cut into the brushed plate instead of painted onto a flat faceplate.
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

void HarmonicBars::paint (juce::Graphics& g)
{
    drawRecessedWell (g, getLocalBounds().toFloat().reduced (1.0f));

    constexpr int N = 512;
    constexpr int K = 7;
    constexpr double twoPi = 6.283185307179586;

    double re[K] = { 0.0 };
    double im[K] = { 0.0 };
    for (int n = 0; n < N; ++n)
    {
        const double xin = 0.7 * std::sin (twoPi * (double) n / (double) N);
        const double y   = wk::wonKnobberTransfer (xin, (double) drive);
        for (int k = 0; k < K; ++k)
        {
            const double a = twoPi * (double) (k + 1) * (double) n / (double) N;
            re[k] += y * std::cos (a);
            im[k] += y * std::sin (a);
        }
    }

    double mag[K];
    for (int k = 0; k < K; ++k)
        mag[k] = std::sqrt (re[k] * re[k] + im[k] * im[k]);
    const double norm = mag[0] > 1.0e-9 ? mag[0] : 1.0;

    const auto b = getLocalBounds().toFloat().reduced (6.0f);
    const float bw = b.getWidth() / (float) K;
    for (int k = 0; k < K; ++k)
    {
        double m = mag[k] / norm;
        m = m < 0.0 ? 0.0 : (m > 1.0 ? 1.0 : m);
        const float h = (float) m * b.getHeight();
        const juce::Rectangle<float> bar (b.getX() + (float) k * bw + bw * 0.22f,
                                          b.getBottom() - h, bw * 0.56f, h);
        g.setColour (juce::Colour (0xffFE9A00).withAlpha (k == 0 ? 1.0f : 0.85f));
        g.fillRect (bar);
    }
}
