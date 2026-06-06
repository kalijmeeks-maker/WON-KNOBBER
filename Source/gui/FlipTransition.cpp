/*
    FlipTransition.cpp — see header. Squash-about-centre approximation of a 180° rotateY flip:
    the outgoing snapshot's horizontal scale runs 1 -> 0 over the first half (turning edge-on at
    90°), then the incoming snapshot runs 0 -> 1 over the second half, with a small vertical squash
    + edge shading for depth. Timing is cubic-bezier(.2,.8,.2,1), no bounce, 450ms.
*/
#include "FlipTransition.h"

#include <cmath>

void FlipTransition::start(juce::Image outgoing, juce::Image incoming, std::function<void()> onDone)
{
    outImg = std::move(outgoing);
    inImg = std::move(incoming);
    done = std::move(onDone);
    startMs = juce::Time::getMillisecondCounterHiRes();
    animating = true;
    setVisible(true);
    toFront(false);
    startTimerHz(60);
    repaint();
}

float FlipTransition::easeBezier(float x)
{
    x = juce::jlimit(0.0f, 1.0f, x);
    auto bez = [](float u, float a, float b)
    {
        const float v = 1.0f - u;
        return 3.0f * v * v * u * a + 3.0f * v * u * u * b + u * u * u;
    };
    // Bisect for the parameter u where the bezier x-coord equals the input x (control xs = .2, .2).
    float lo = 0.0f, hi = 1.0f, u = x;
    for (int i = 0; i < 24; ++i)
    {
        u = 0.5f * (lo + hi);
        if (bez(u, 0.2f, 0.2f) < x)
            lo = u;
        else
            hi = u;
    }
    return bez(u, 0.8f, 1.0f); // control ys = .8, 1.0
}

void FlipTransition::timerCallback()
{
    const double elapsed = juce::Time::getMillisecondCounterHiRes() - startMs;
    if (elapsed >= kDurationMs)
    {
        animating = false;
        stopTimer();
        repaint();
        if (done)
        {
            auto cb = done;
            done = nullptr;
            cb();
        }
        return;
    }
    repaint();
}

void FlipTransition::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1b1b1e)); // editor backdrop behind the flipping card

    if (!animating)
        return;

    const double elapsed = juce::Time::getMillisecondCounterHiRes() - startMs;
    const float t = (float)juce::jlimit(0.0, 1.0, elapsed / kDurationMs);
    const float eased = easeBezier(t);

    const float angle = juce::MathConstants<float>::pi * eased; // 0..pi  (0°..180°)
    const float c = std::cos(angle);                           // 1..-1
    const float sx = juce::jmax(0.02f, std::abs(c));           // horizontal squash toward edge-on
    const float sy = 1.0f - 0.10f * std::sin(angle);           // slight vertical squash for depth

    const juce::Image& img = (angle <= juce::MathConstants<float>::halfPi) ? outImg : inImg;
    if (!img.isValid())
        return;

    const auto b = getLocalBounds().toFloat();
    // Map the snapshot's pixels onto the panel bounds (handles HiDPI snapshot sizes), then squash
    // horizontally + vertically about the panel centre to fake the rotateY.
    const auto base = juce::AffineTransform::scale(b.getWidth() / (float)img.getWidth(),
                                                   b.getHeight() / (float)img.getHeight());
    const auto flip = juce::AffineTransform::scale(sx, sy, b.getCentreX(), b.getCentreY());
    g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
    g.drawImageTransformed(img, base.followedBy(flip));

    // Darken as the card turns edge-on (subtle 3D shading).
    g.setColour(juce::Colours::black.withAlpha(0.35f * std::sin(angle)));
    g.fillRect(getLocalBounds());
}
