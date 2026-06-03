/*
    PresetManager.h — disk-backed user preset bank (v1.1)
    WON-KNOBBER · preset I/O layer (message thread only — never touch from processBlock)

    A user preset is exactly a WonKnobberState serialised as the same ValueTree XML the
    embedded factory voices use, written to "<name>.wknob" under a platform user-data dir.
    This keeps one serialisation format across host-state, factory presets, and user presets.
*/
#pragma once

#include "WonKnobberState.h"

#include <juce_core/juce_core.h>

class PresetManager
{
public:
    // Extension (incl. dot) for on-disk user presets.
    static constexpr const char* kFileExtension = ".wknob";

    // Defaults to the platform user-preset directory (see defaultDirectory()). Tests inject a
    // temp dir. Construction is read-only: it scans the dir if present but never creates it.
    explicit PresetManager(juce::File directory = defaultDirectory());

    // Platform user-preset folder: <userAppData>[/Application Support]/WON-KNOBBER/Presets.
    // Pure path math — does not touch the filesystem.
    static juce::File defaultDirectory();

    // The directory user presets live in (created lazily on first successful save).
    juce::File getPresetDirectory() const { return presetDir; }

    // Re-scan the directory for *.wknob files; sorted case-insensitively by display name.
    // Read-only: a missing directory yields an empty list, no creation.
    void refresh();

    int getNumPresets() const { return presetFiles.size(); }

    // Display name = filename without extension. {} if index out of range.
    juce::String getPresetName(int index) const;

    // Index of the preset whose display name matches (case-insensitive), or -1.
    int indexOf(const juce::String& name) const;

    // Save `state` to "<sanitised name>.wknob" (overwriting any existing file of that name),
    // creating the directory if needed, then refresh(). Empty/whitespace name -> false.
    bool savePreset(const juce::String& name, const WonKnobberState& state);

    // Load preset at `index` into `state`. Returns false on out-of-range / parse failure
    // (state untouched on failure). Parsed fields are sanitised via WonKnobberState::fromValueTree.
    bool loadPreset(int index, WonKnobberState& state) const;

    // Move preset at `index` to the OS trash (recoverable), then refresh(). False if oob / fails.
    bool deletePreset(int index);

    // Legalise a user-entered name into a safe single-segment filename stem (no path separators,
    // no extension). Empty/whitespace -> "Untitled". Exposed for the UI to preview/echo.
    static juce::String sanitizeName(const juce::String& raw);

private:
    juce::File presetDir;
    juce::Array<juce::File> presetFiles; // sorted by display name (case-insensitive)

    JUCE_LEAK_DETECTOR(PresetManager)
};
