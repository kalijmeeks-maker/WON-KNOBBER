/*
    FlipTransition.h — animated front<->rear "flip" overlay. Plays a 180° rotateY card-flip between
    a snapshot of the outgoing side and the incoming side, over 450ms with cubic-bezier(.2,.8,.2,1),
    no bounce (Design's locked transition spec). While animating it sits on top + intercepts clicks;
    on completion it hides itself and the editor shows the incoming side. Reduced-motion callers should
    skip start() and swap instantly.
    WON-KNOBBER · part of the gui layer
*/
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class FlipTransition : public juce::Component, private juce::Timer
{
public:
    FlipTransition() { setInterceptsMouseClicks(true, false); }

    // Begin the flip. `outgoing`/`incoming` are full-component snapshots; onDone fires when finished.
    void start(juce::Image outgoing, juce::Image incoming, std::function<void()> onDone);
    bool isAnimating() const noexcept { return animating; }

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    static float easeBezier(float x); // cubic-bezier(.2,.8,.2,1): solve param for x, return y

    juce::Image outImg, inImg;
    std::function<void()> done;
    double startMs{0.0};
    bool animating{false};
    static constexpr double kDurationMs = 450.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlipTransition)
};
