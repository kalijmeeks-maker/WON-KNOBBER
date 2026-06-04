/*
    PresetManager.cpp — see header. Disk I/O is message-thread only. Self-tests (temp dir) run on dylib load.
*/
#include "PresetManager.h"

#include <cmath>
#include <iostream>

namespace
{
// Case-insensitive sort of the on-disk bank by display name (filename without extension).
struct PresetNameComparator
{
    int compareElements(const juce::File& a, const juce::File& b) const
    {
        return a.getFileNameWithoutExtension().compareIgnoreCase(b.getFileNameWithoutExtension());
    }
};
} // namespace

PresetManager::PresetManager(juce::File directory) : presetDir(std::move(directory))
{
    refresh();
}

juce::File PresetManager::defaultDirectory()
{
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    base = base.getChildFile("Application Support");
#endif
    return base.getChildFile("WON-KNOBBER").getChildFile("Presets");
}

void PresetManager::refresh()
{
    presetFiles.clearQuick();

    if (presetDir.isDirectory())
    {
        presetFiles = presetDir.findChildFiles(juce::File::findFiles, false, juce::String("*") + kFileExtension);
        PresetNameComparator cmp;
        presetFiles.sort(cmp);
    }
}

juce::String PresetManager::getPresetName(int index) const
{
    if (index < 0 || index >= presetFiles.size())
        return {};
    return presetFiles[index].getFileNameWithoutExtension();
}

int PresetManager::indexOf(const juce::String& name) const
{
    for (int i = 0; i < presetFiles.size(); ++i)
        if (presetFiles[i].getFileNameWithoutExtension().equalsIgnoreCase(name))
            return i;
    return -1;
}

bool PresetManager::savePreset(const juce::String& name, const WonKnobberState& state)
{
    const juce::String stem = sanitizeName(name);
    if (stem.isEmpty())
        return false;

    if (!presetDir.isDirectory())
    {
        const auto res = presetDir.createDirectory();
        if (res.failed())
            return false;
    }

    auto xml = state.toValueTree().createXml();
    if (xml == nullptr)
        return false;

    const juce::File file = presetDir.getChildFile(stem + kFileExtension);
    if (!file.replaceWithText(xml->toString()))
        return false;

    refresh();
    return true;
}

bool PresetManager::loadPreset(int index, WonKnobberState& state) const
{
    if (index < 0 || index >= presetFiles.size())
        return false;

    auto xml = juce::XmlDocument::parse(presetFiles[index]);
    if (xml == nullptr)
        return false;

    state = WonKnobberState::fromValueTree(juce::ValueTree::fromXml(*xml));
    return true;
}

bool PresetManager::deletePreset(int index)
{
    if (index < 0 || index >= presetFiles.size())
        return false;

    // Move to the OS trash (recoverable) rather than a permanent unlink — matches the header
    // contract and gives the user a safety net behind the editor's delete-confirm dialog.
    const bool ok = presetFiles[index].moveToTrash();
    refresh();
    return ok;
}

juce::String PresetManager::sanitizeName(const juce::String& raw)
{
    const juce::String trimmed = raw.trim();
    if (trimmed.isEmpty())
        return "Untitled";

    juce::String legal = juce::File::createLegalFileName(trimmed).trim();

    // Drop a user-typed copy of our own extension so "My Tone.wknob" stores as "My Tone".
    if (legal.endsWithIgnoreCase(kFileExtension))
        legal = legal.dropLastCharacters(juce::String(kFileExtension).length()).trim();

    return legal.isEmpty() ? juce::String("Untitled") : legal;
}

//==============================================================================
// Self-tests — DEBUG builds only. They do disk I/O at static init / dlopen, and the [delete] case
// now exercises moveToTrash(), so running them in a shipping plugin would litter the user's Trash on
// every load. Gated behind JUCE_DEBUG; run a Debug build to see the PRESETMGR PASS lines.
// Uses a unique temp directory and cleans up after itself so the real user bank is never touched.
#if JUCE_DEBUG
namespace
{
static bool runPresetManagerTests()
{
    bool pass = true;

    const auto tmpRoot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                             .getChildFile("wonknobber_pm_test_" + juce::String(juce::Time::currentTimeMillis()));
    tmpRoot.deleteRecursively(); // clean slate

    PresetManager pm(tmpRoot);

    // Empty start: no presets, and ctor must NOT have created the directory.
    {
        const bool ok = (pm.getNumPresets() == 0) && !tmpRoot.isDirectory();
        std::cout << "PRESETMGR [empty start]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Save creates the dir + file; round-trips every field through ValueTree XML.
    {
        WonKnobberState st;
        st.drive = 0.31f;
        st.mix = 0.66f;
        st.variant = "ruby";
        st.bypass = true;
        st.cabIr = "VINTAGE_4X12";
        st.neuralModel = "IRON";
        st.cabEngage = true;
        st.neuralEngage = true;

        const bool saved = pm.savePreset("My Tone", st);

        WonKnobberState back;
        const int idx = pm.indexOf("My Tone");
        const bool loaded = (idx >= 0) && pm.loadPreset(idx, back);
        const bool fields = std::abs(back.drive - 0.31f) < 1e-4f && std::abs(back.mix - 0.66f) < 1e-4f &&
                            back.variant == "ruby" && back.bypass && back.cabIr == "VINTAGE_4X12" &&
                            back.neuralModel == "IRON" && back.cabEngage && back.neuralEngage;
        const bool ok = saved && tmpRoot.isDirectory() && pm.getNumPresets() == 1 && loaded && fields;
        std::cout << "PRESETMGR [save+load roundtrip]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Overwriting the same name keeps the bank size at one.
    {
        WonKnobberState st;
        st.drive = 0.9f;
        pm.savePreset("My Tone", st);
        const bool ok = pm.getNumPresets() == 1;
        std::cout << "PRESETMGR [overwrite keeps count]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Path traversal / illegal chars are stripped; empty -> "Untitled".
    {
        const auto safe = PresetManager::sanitizeName("../../etc/passwd");
        const bool ok = !safe.containsChar('/') && !safe.containsChar('\\') && safe.isNotEmpty();
        std::cout << "PRESETMGR [sanitize traversal]: " << (ok ? "PASS" : "FAIL") << " -> " << safe << std::endl;
        if (!ok)
            pass = false;

        const bool emptyOk = PresetManager::sanitizeName("   ") == "Untitled";
        std::cout << "PRESETMGR [sanitize empty]: " << (emptyOk ? "PASS" : "FAIL") << std::endl;
        if (!emptyOk)
            pass = false;
    }

    // Second preset + case-insensitive sorted listing.
    {
        WonKnobberState st;
        pm.savePreset("Alpha", st);
        const bool ok =
            pm.getNumPresets() == 2 && pm.getPresetName(0) == "Alpha" && pm.getPresetName(1) == "My Tone";
        std::cout << "PRESETMGR [sorted listing]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    // Delete drops the count and removes it from lookups.
    {
        const int idx = pm.indexOf("Alpha");
        const bool del = (idx >= 0) && pm.deletePreset(idx);
        const bool ok = del && pm.getNumPresets() == 1 && pm.indexOf("Alpha") < 0;
        std::cout << "PRESETMGR [delete]: " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok)
            pass = false;
    }

    tmpRoot.deleteRecursively(); // cleanup

    std::cout << "PRESETMGR TESTS OVERALL: " << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

static const bool presetManagerTestsRan = runPresetManagerTests();
} // namespace
#endif // JUCE_DEBUG
