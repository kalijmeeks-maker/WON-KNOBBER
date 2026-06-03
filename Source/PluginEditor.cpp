/*
    PluginEditor.cpp — see header. Wires the drive knob to the AudioParameterFloat.
*/
#include "PluginEditor.h"

#include <juce_core/juce_core.h>

#include <cmath>

WonKnobberAudioProcessorEditor::WonKnobberAudioProcessorEditor(WonKnobberAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processorRef(p)
{
    addAndMakeVisible(faceplate);

    auto& knob = faceplate.getDriveKnob();
    if (auto* driveParam = processorRef.getDriveParameter())
    {
        knob.setValue(driveParam->get(), juce::dontSendNotification);
        knob.onValueChange = [this, &knob, driveParam]
        {
            *driveParam = (float)knob.getValue();
            faceplate.setDrive((float)knob.getValue());
        };
        faceplate.setDrive(driveParam->get());
    }

    auto& mixKnob = faceplate.getMixKnob();
    if (auto* mixParam = processorRef.getMixParameter())
    {
        mixKnob.setValue(mixParam->get(), juce::dontSendNotification);
        mixKnob.onValueChange = [this, &mixKnob, mixParam]
        {
            *mixParam = (float)mixKnob.getValue();
            juce::ignoreUnused(this);
        };
    }

    // Restore the persisted stone (defaults to "diamond" on a fresh load) and
    // let the chip write any future picker change back to the processor.
    faceplate.setVariant(processorRef.getCurrentVariant());
    faceplate.onVariantPicked = [this](const juce::String& stone) { processorRef.setCurrentVariant(stone); };

    // Phase 2b: preset strip (factory name LED + ‹› cycle + A/B) + transport tray (S/L/U/R).
    // Init display (preset name is UI-transient, not persisted in state; start on the first
    // factory voice; updates only on explicit factory load via strip).
    faceplate.setNumFactoryPresets(processorRef.getNumFactoryPresets());
    faceplate.setPresetDisplayName(processorRef.getFactoryPresetName(0));
    faceplate.setActiveSlot(processorRef.getActiveSlot());

    faceplate.onFactoryPresetSelected = [this](int idx)
    {
        processorRef.loadFactoryPreset(idx);
        faceplate.setVariant(processorRef.getCurrentVariant());
        if (auto* dp = processorRef.getDriveParameter())
            faceplate.setDrive(dp->get());
        if (auto* mp = processorRef.getMixParameter())
        {
            auto& mk = faceplate.getMixKnob();
            mk.setValue(mp->get(), juce::dontSendNotification);
        }
        faceplate.setPresetDisplayName(processorRef.getFactoryPresetName(idx));
        faceplate.setActiveSlot(processorRef.getActiveSlot());
    };

    faceplate.onActiveSlotSelected = [this](char slot)
    {
        processorRef.setActiveSlot(slot);
        faceplate.setVariant(processorRef.getCurrentVariant());
        if (auto* dp = processorRef.getDriveParameter())
            faceplate.setDrive(dp->get());
        if (auto* mp = processorRef.getMixParameter())
        {
            auto& mk = faceplate.getMixKnob();
            mk.setValue(mp->get(), juce::dontSendNotification);
        }
        faceplate.setActiveSlot(slot);
    };

    faceplate.onTransportAction = [this](const juce::String& action)
    {
        if (action == "save")
        {
            processorRef.saveToActiveSlot();
        }
        else if (action == "load")
        {
            processorRef.loadFromActiveSlot();
            faceplate.setVariant(processorRef.getCurrentVariant());
            if (auto* dp = processorRef.getDriveParameter())
                faceplate.setDrive(dp->get());
            if (auto* mp = processorRef.getMixParameter())
            {
                auto& mk = faceplate.getMixKnob();
                mk.setValue(mp->get(), juce::dontSendNotification);
            }
        }
        else if (action == "undo")
        {
            processorRef.undoLast();
            faceplate.setVariant(processorRef.getCurrentVariant());
            if (auto* dp = processorRef.getDriveParameter())
                faceplate.setDrive(dp->get());
            if (auto* mp = processorRef.getMixParameter())
            {
                auto& mk = faceplate.getMixKnob();
                mk.setValue(mp->get(), juce::dontSendNotification);
            }
            faceplate.setActiveSlot(processorRef.getActiveSlot());
        }
        else if (action == "randomize")
        {
            processorRef.randomizeParameters();
            faceplate.setVariant(processorRef.getCurrentVariant());
            if (auto* dp = processorRef.getDriveParameter())
                faceplate.setDrive(dp->get());
            if (auto* mp = processorRef.getMixParameter())
            {
                auto& mk = faceplate.getMixKnob();
                mk.setValue(mp->get(), juce::dontSendNotification);
            }
        }
    };

    // Modified-from-preset dot: clicking it reverts the live state to the loaded voice, then refreshes
    // the GUI exactly like the load/undo transport actions do (knob/mix/variant pulled back from processor).
    faceplate.onRevertToPreset = [this]
    {
        processorRef.revertToLoadedPreset();
        faceplate.setVariant(processorRef.getCurrentVariant());
        if (auto* dp = processorRef.getDriveParameter())
            faceplate.setDrive(dp->get());
        if (auto* mp = processorRef.getMixParameter())
        {
            auto& mk = faceplate.getMixKnob();
            mk.setValue(mp->get(), juce::dontSendNotification);
        }
        faceplate.setModified(processorRef.isDirty());
    };

    setSize(960, 600);
    startTimerHz(30);
}

void WonKnobberAudioProcessorEditor::timerCallback()
{
    if (auto* driveParam = processorRef.getDriveParameter())
    {
        const float d = driveParam->get();
        auto& knob = faceplate.getDriveKnob();
        if (std::abs((float)knob.getValue() - d) > 1.0e-4f)
            knob.setValue(d, juce::dontSendNotification); // reflect host automation on knob + arc
        faceplate.setDrive(d);
    }

    if (auto* mixParam = processorRef.getMixParameter())
    {
        auto& knob = faceplate.getMixKnob();
        const float m = mixParam->get();
        if (std::abs((float)knob.getValue() - m) > 1.0e-4f)
            knob.setValue(m, juce::dontSendNotification);
    }

    // Sync variant (non-automatable) in case a preset load or slot switch happened
    // while editor is open (e.g. host recall while UI visible, or future external control).
    {
        const auto v = processorRef.getCurrentVariant();
        if (v != faceplate.getVariant())
            faceplate.setVariant(v);
    }

    // Reflect active slot (mostly driven by our strip clicks, but keeps indicator correct
    // after any processor-driven change).
    faceplate.setActiveSlot(processorRef.getActiveSlot());

    // Light the "modified-from-preset" ember dot whenever the live state diverges from the loaded voice
    // (manual knob edits, host automation, etc.). FaceplateView no-ops if the flag is unchanged.
    faceplate.setModified(processorRef.isDirty());

    // Drain the audio-thread peak accumulators and feed them to the I/O meter
    // with a real elapsed-time delta so the hold/decay ballistics are frame-rate
    // independent (clamped to a sane upper bound in case the timer stalls).
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = lastTickSec > 0.0 ? (float)juce::jlimit(0.0, 0.25, now - lastTickSec) : 1.0f / 30.0f;
    lastTickSec = now;

    const auto peaks = processorRef.consumeMeterPeaks();
    faceplate.pushLevels(peaks.inL, peaks.inR, peaks.outL, peaks.outR, dt);
}

void WonKnobberAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1b1b1e));
}

void WonKnobberAudioProcessorEditor::resized()
{
    faceplate.setBounds(getLocalBounds());
}
