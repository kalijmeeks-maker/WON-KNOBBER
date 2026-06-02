/*
    HarmonicBars.cpp — see header. DFT of a sine driven through the active transfer;
    bars 1..7 normalised to the fundamental, drawn over the chassis HARMONICS panel.
*/
#include "HarmonicBars.h"

#include "../dsp/TransferModel.h"

#include <cmath>

void HarmonicBars::paint (juce::Graphics& g)
{
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
