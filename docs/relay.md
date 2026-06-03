# Relay log

### 2026-06-03 15:00 FROM human TO claude
DID: Enabled autonomous relay on WON-KNOBBER (`AGENTS.md`, this mailbox, `wk relay`).
HANDOFF: You are first mover on `agent/claude`. Read `AGENTS.md` lanes. Pick ONE
scoped goal and execute: (A) v1.1 user preset bank — `PresetManager` + APVTS snapshot
+ disk I/O under `Source/Presets/`, or (B) any open completion handoff from `main`
from `main` (flip spec polish, preset menu UI) if PresetManager already exists.
Commit only your lane files. Append relay + commit `docs/relay.md`. HANDOFF grok
with concrete files/paths for the UI lane.
NEEDS: none

### 2026-06-03 16:18 FROM claude TO grok
DID: Built the v1.1 user preset bank BACKEND (goal A — no PresetManager existed).
  - NEW `Source/Presets/PresetManager.{h,cpp}`: disk-backed bank reusing the existing
    `WonKnobberState` ValueTree XML (SAME format as factory presets + host state — no new
    serialisation). API: refresh / getNumPresets / getPresetName / indexOf / savePreset /
    loadPreset / deletePreset / sanitizeName + static defaultDirectory()
    (`<userAppData>[/Application Support]/WON-KNOBBER/Presets`). Message-thread ONLY; never
    touches processBlock. Names legalised via createLegalFileName (path-traversal safe).
    Self-tests run on dlopen against a TEMP dir (cleaned up): empty-start, save+load roundtrip
    of all 8 fields, overwrite-keeps-count, sanitize (traversal + empty), sorted listing, delete.
  - WIRED into `Source/PluginProcessor.{h,cpp}`: getNumUserPresets / getUserPresetName(i) /
    saveUserPreset(name) / loadUserPreset(i) / deleteUserPreset(i) / refreshUserPresets /
    findUserPresetIndex(name) / getUserPresetDirectory(). save snapshots getCurrentState();
    load calls applyState (resets A/B slots + active='A' + re-stamps the dirty baseline —
    IDENTICAL to factory-load semantics).
  - NOTE: handoff said "APVTS snapshot" but this repo uses the custom WonKnobberState snapshot,
    NOT APVTS. Built on the real source-of-truth — don't go hunting for an APVTS layer.
  - BUILD: cmake Debug configure + build PASS, ZERO warnings under -Werror, AU + VST3 linked.
    Could NOT dlopen-exec the self-tests (sandbox blocks running the built binary); they compile
    clean and follow the repo's existing static-init PASS/FAIL pattern — please eyeball the
    PRESETMGR lines on your next local plugin load.

HANDOFF: grok — UI LANE ONLY. Expose the bank via a PopupMenu on the preset-name LED.
  Do NOT touch Source/Presets/* or PluginProcessor.* (my lane, locked). UI-only edits below.

  1) `Source/gui/FaceplateView.h` + `.cpp`
     - Add public callback near the other preset callbacks (~h:74-77):
         std::function<void()> onPresetMenuRequested;
     - In `FaceplateView::mouseDown` footer hit-test: when the click is inside `presetNameBounds`
       (the engraved name LED) AND NOT inside chevLeftBounds/chevRightBounds/modifiedDotBounds,
       fire onPresetMenuRequested() if set. Leave the chevrons firing cyclePreset as-is.
     - Optional: draw a tiny ▾ caret beside the name in drawPresetStrip to signal it's clickable.

  2) `Source/PluginEditor.h` + `.cpp`
     - Add private: void showPresetMenu(); void handlePresetMenuResult(int); and
       void applyLoadedStateToGui(); — move the copy-pasted post-load GUI sync (variant/drive/mix
       pulled back from processorRef) out of onFactoryPresetSelected (cpp:58-71),
       onActiveSlotSelected (73-85), onTransportAction (87-130) into applyLoadedStateToGui()
       and call it from each. Reuse that EXACT body (incl. faceplate.setModified(processorRef.isDirty())).
     - Ctor: faceplate.onPresetMenuRequested = [this]{ showPresetMenu(); };
     - showPresetMenu():
         processorRef.refreshUserPresets();
         juce::PopupMenu menu;  // header "FACTORY" + items id=1..N (getFactoryPresetName i);
         // header "USER" + items id=1000+i (getUserPresetName i), or disabled "(no user presets)";
         // separator; "Save As…" id=2001; "Delete \"<current>\"…" id=2002 ONLY if
         // processorRef.findUserPresetIndex(currentDisplayName) >= 0; "Reveal Presets Folder" id=2003.
         menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
                            [this](int r){ handlePresetMenuResult(r); });
     - handlePresetMenuResult(r):
         * 1..N      -> loadFactoryPreset(r-1); setPresetDisplayName(getFactoryPresetName(r-1));
                        applyLoadedStateToGui(); faceplate.setActiveSlot(getActiveSlot());
         * 1000.. -> idx=r-1000; if (loadUserPreset(idx)) { setPresetDisplayName(getUserPresetName(idx));
                        applyLoadedStateToGui(); faceplate.setActiveSlot(getActiveSlot()); }
         * 2001 (Save As) -> juce::AlertWindow text entry ("Preset name", default=current name),
                        showAsync; on OK: auto n = aw.getTextEditorContents("name");
                        if (processorRef.saveUserPreset(n)) setPresetDisplayName(PresetManager::sanitizeName(n));
                        (stored stem is SANITISED — use sanitizeName for the echo, or re-read via findUserPresetIndex.)
         * 2002 (Delete) -> juce::AlertWindow::showOkCancelBox confirm; on OK
                        idx=findUserPresetIndex(currentName); if(idx>=0) deleteUserPreset(idx);
         * 2003 -> auto d=processorRef.getUserPresetDirectory(); d.createDirectory(); d.revealToUser();
     - Includes: PluginEditor.cpp already pulls PluginProcessor.h -> Presets/PresetManager.h, so
       PresetManager::sanitizeName is in scope.

  CONSTRAINTS: all preset I/O is message-thread (you're in the editor — fine); never call
  save/load/delete from the 30 Hz timer. Keep the build ZERO-warnings under -Werror (watch
  unused lambda captures). No new layout anchors needed — presetNameBounds already exists.

NEEDS: none (backend complete + builds clean; UI is unblocked).