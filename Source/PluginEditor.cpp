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

    // Bypass rocker: reflect persisted state, write toggles back to the processor.
    faceplate.setBypassed(processorRef.getBypassState());
    faceplate.onBypassToggled = [this](bool b)
    {
        processorRef.setBypassState(b);
        faceplate.setBypassed(b);
    };

    // Phase 2b: preset strip (factory name LED + ‹› cycle + A/B) + transport tray (S/L/U/R).
    // Init display (preset name is UI-transient, not persisted in state; start on the first
    // factory voice; updates only on explicit factory load via strip).
    faceplate.setNumFactoryPresets(processorRef.getNumFactoryPresets());
    faceplate.setPresetDisplayName(processorRef.getFactoryPresetName(0));
    faceplate.setActiveSlot(processorRef.getActiveSlot());
    faceplate.setModified(processorRef.isDirty());  // initial dot (was only set on first timer tick)

    faceplate.onFactoryPresetSelected = [this](int idx)
    {
        processorRef.loadFactoryPreset(idx);
        faceplate.setPresetDisplayName(processorRef.getFactoryPresetName(idx));
        faceplate.setActiveSlot(processorRef.getActiveSlot());
        applyLoadedStateToGui();
    };

    faceplate.onActiveSlotSelected = [this](char slot)
    {
        processorRef.setActiveSlot(slot);
        faceplate.setActiveSlot(slot);
        applyLoadedStateToGui();
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
            applyLoadedStateToGui();
            faceplate.setActiveSlot(processorRef.getActiveSlot());
        }
        else if (action == "undo")
        {
            processorRef.undoLast();
            applyLoadedStateToGui();
            faceplate.setActiveSlot(processorRef.getActiveSlot());
        }
        else if (action == "randomize")
        {
            processorRef.randomizeParameters();
            applyLoadedStateToGui();
            faceplate.setActiveSlot(processorRef.getActiveSlot());
        }
    };

    // Modified-from-preset dot: clicking it reverts the live state to the loaded voice, then refreshes
    // the GUI exactly like the load/undo transport actions do (knob/mix/variant pulled back from processor).
    faceplate.onRevertToPreset = [this]
    {
        processorRef.revertToLoadedPreset();
        applyLoadedStateToGui();
    };

    faceplate.onPresetMenuRequested = [this]{ showPresetMenu(); };

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

    // Reflect bypass (e.g. host state recall while UI open); setBypassed no-ops if unchanged.
    faceplate.setBypassed(processorRef.getBypassState());

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

void WonKnobberAudioProcessorEditor::applyLoadedStateToGui()
{
    faceplate.setVariant(processorRef.getCurrentVariant());
    if (auto* dp = processorRef.getDriveParameter())
        faceplate.setDrive(dp->get());
    if (auto* mp = processorRef.getMixParameter())
    {
        auto& mk = faceplate.getMixKnob();
        mk.setValue(mp->get(), juce::dontSendNotification);
    }
    faceplate.setModified(processorRef.isDirty());
}

void WonKnobberAudioProcessorEditor::showPresetMenu()
{
    processorRef.refreshUserPresets();

    juce::PopupMenu menu;

    // FACTORY section
    menu.addSectionHeader("FACTORY");
    const int nFactory = processorRef.getNumFactoryPresets();
    for (int i = 0; i < nFactory; ++i)
        menu.addItem(1 + i, processorRef.getFactoryPresetName(i));

    // USER section
    menu.addSectionHeader("USER");
    const int nUser = processorRef.getNumUserPresets();
    if (nUser == 0)
    {
        menu.addItem(1000, "(no user presets)", false, false);
    }
    else
    {
        for (int i = 0; i < nUser; ++i)
            menu.addItem(1000 + i, processorRef.getUserPresetName(i));
    }

    // actions
    menu.addSeparator();
    menu.addItem(2001, "Save As…");

    const juce::String currentDisp = faceplate.getPresetDisplayName();
    const int userIdxForCurrent = processorRef.findUserPresetIndex(currentDisp);
    const bool canDeleteCurrent = userIdxForCurrent >= 0;
    menu.addItem(2002, juce::String("Delete \"") + currentDisp + "\"…", canDeleteCurrent);

    menu.addItem(2003, "Reveal Presets Folder");

    menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
                       [this](int r){ handlePresetMenuResult(r); });
}

void WonKnobberAudioProcessorEditor::handlePresetMenuResult(int r)
{
    // Menu ID scheme (to avoid magic/overlap): factory 1..N (N<1000), user 1000..1999 (1000 used for disabled placeholder when no users),
    // actions 2001=SaveAs, 2002=Delete (enabled only for current user preset), 2003=Reveal.
    // Factory if tightened to <1000 so r=1000 (disabled) does not oob into factory idx=999.
    if (r >= 1 && r < 1000)
    {
        const int idx = r - 1;
        processorRef.loadFactoryPreset(idx);
        applyLoadedStateToGui();
        faceplate.setPresetDisplayName(processorRef.getFactoryPresetName(idx));
        faceplate.setActiveSlot(processorRef.getActiveSlot());
    }
    else if (r >= 1000 && r < 2000)
    {
        const int idx = r - 1000;
        if (processorRef.loadUserPreset(idx))
        {
            applyLoadedStateToGui();
            faceplate.setPresetDisplayName(processorRef.getUserPresetName(idx));
            faceplate.setActiveSlot(processorRef.getActiveSlot());
        }
    }
    else if (r == 2001)
    {
        // Save As…
        juce::AlertWindow aw("Save Preset", "Preset name:", juce::AlertWindow::QuestionIcon);
        aw.addTextEditor("name", currentDisp, "Name:");  // use current for default (was hard "My Preset")
        aw.addButton("OK", 1);
        aw.addButton("Cancel", 0);
        // Capture TextEditor* before enterModalState (avoids fragile getCurrentlyModalComponent + global state in async callback).
        if (auto* nameEd = aw.getTextEditor("name"))
        {
            aw.enterModalState(true, juce::ModalCallbackFunction::create([this, nameEd](int result)
            {
                if (result == 0) return;
                if (!nameEd) return;
                auto n = nameEd->getText();
                if (n.isEmpty()) return;
                if (processorRef.saveUserPreset(n))
                {
                    // Use PresetManager::sanitizeName (visible via Processor.h include) to match bank name exactly.
                    // Removes prior local ad-hoc duplication + edge divergence (whitespace, ext, fallback "Untitled").
                    juce::String s = PresetManager::sanitizeName(n);
                    faceplate.setPresetDisplayName(s);
                    applyLoadedStateToGui();
                }
            }));
        }
    }
    else if (r == 2002)
    {
        // Delete only for user presets (gated by menu item enabled state + recheck). Single clean confirm (no nesting).
        const juce::String cur = faceplate.getPresetDisplayName();
        const int idx = processorRef.findUserPresetIndex(cur);
        if (idx >= 0)
        {
            juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon, "Delete Preset",
                juce::String("Delete \"") + cur + "\"?", "Delete", "Cancel", this,
                juce::ModalCallbackFunction::create([this, idx, cur](int res)
                {
                    if (res == 0) return;
                    // Re-validate idx still valid (defensive); delete moves to trash (c3e16a4).
                    if (processorRef.findUserPresetIndex(cur) >= 0)
                        processorRef.deleteUserPreset(idx);
                    applyLoadedStateToGui();
                    // Fix stale display name post-delete: snap to first factory (valid name). User can cycle/load next.
                    if (processorRef.getNumFactoryPresets() > 0)
                        faceplate.setPresetDisplayName(processorRef.getFactoryPresetName(0));
                }));
        }
    }
    else if (r == 2003)
    {
        auto d = processorRef.getUserPresetDirectory();
        d.createDirectory();
        d.revealToUser();
    }
}
