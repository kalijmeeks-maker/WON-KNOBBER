/*
    TransferCurve.cpp — see header. Transparent over the chassis TRANSFER panel:
    faint grid + unity diagonal + amber transfer curve from wk::wonKnobberTransfer.
*/
#include "TransferCurve.h"

#include "../dsp/TransferModel.h"

void TransferCurve::paint (juce::Graphics& g)
{
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
