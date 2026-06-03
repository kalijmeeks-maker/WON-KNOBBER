# PREP_INTEGRATION_PLAN.md — WON-KNOBBER Cab/Neural Handoff Integration

**Status:** Prepared 2026-06-03 (read-only architect review of current main + all docs + Source).  
**Purpose:** Turnkey checklist + sites + risks + tests + ID maps so the next handoff (final locked IDs, reconciled map/manifest, possibly updated anchors or drive/mix) can be executed as a single focused PR with zero exploration.  
**Context (from voice-cab-neural-map.md + manifest + bridge + integration_spec + code):**  
- Authoritative voice→(cabIr, neuralModel, cabEngage, neuralEngage) is in `docs/voice-cab-neural-map.md` (Claude table, uses *display* names with spaces + TAPE-1971 etc.).  
- Machine manifest + preset_voice_map in `docs/WON_cab_neural_asset_manifest_2026-06-02.json` + `docs/cab-neural-id-bridge.json` use *state_id* (underscores + short neural: STUDIO_RIBBON / TAPE etc.).  
- Current implementation + 8 XMLs + self-tests + DSP lookups + processor use the **state_id / manifest "id"** set.  
- Rear UI (flip-spec.html + rear-panel-anchors.json) describes painting the *display* names (STUDIO RIBBON / TAPE-1971 / VINTAGE 4x12 etc.).  
- RT-safe contract (message-thread load + atomic/ JUCE wait-free swap + zero-latency + PDC stable + bit-exact disengage passthrough) is already wired in state/processor/Convolution/NeuralModel.  
- Next handoff likely: "final locked ID strings", sync of drive/mix or map, rear anchors PNG landed, "make presets + code match exactly", full cycle test.

The plan is **prioritized for immediate PR** on arrival. All changes are additive or exact-string syncs; no DSP math changes.

## 1. Prioritized Checklist (what must happen on handoff arrival)

**P0 — ID reconciliation + string sync (CRITICAL — do first, 1-2h effort)**  
1. Receive final decision on canonical keys for `WonKnobberState` / XML attrs / isKnown* / DSP lookups (display names from `voice-cab-neural-map.md` table like `STUDIO RIBBON` / `TAPE-1971` vs current state_ids `STUDIO_RIBBON` / `TAPE` from bridge + manifest "id"). Ask CC/human explicitly if not stated (see §5).  
2. Update **every** occurrence of the 4 cab + 5 neural strings (see ID audit map below) to the *final locked set*. Includes:  
   - `isKnownCabIr` / `isKnownNeural` + StringArray lists.  
   - `irDataForId` + `modelDataForId`.  
   - All struct defaults, VT getProperty defaults, sanitize fallbacks.  
   - Member inits in Processor + State tests + DSP smoke tests.  
   - All 8 `Resources/factory_presets/*.xml` (use bridge or map to pick the final string form + authoritative drive/mix + engages from the voice table).  
   - Comments in headers (e.g. "FLAT/STUDIO_RIBBON/...").  
3. If final IDs are the *display/spaced* set, also update the dataForId functions to key on display (or add a thin internal->display map + keep BinaryData lookups on filename-derived symbols). BinaryData symbols **never change** (filenames fixed).  
4. Reconcile `voice-cab-neural-map.md` vs `cab-neural-id-bridge.json` vs manifest "preset_voice_map_authoritative" (the table in map wins for drive/mix/cab assignments; bridge for display<->state translation if UI uses display). Update the example XML in manifest + integration_spec.  
5. Run `cmake -B build ... && cmake --build build` — must be zero warnings. (CI will fail otherwise.)

**P1 — Verify + extend wiring + PDC/latency (1-2h)**  
6. In `PluginProcessor::prepareToPlay`: compute + set full PDC: `saturation.getLatencySamples() + getConvLatency() + 0` (neural always 0 per spec). Expose `int getLatencySamples() const` on Convolution wrapper (delegates to internal `convolution.getLatency()`). Call after conv.prepare + after any setIr in prepare path.  
7. On IR swap path (`applyStateToParams` after `convolution.setIr`): only call `setLatencySamples` **if** the new conv latency actually differs from prior (v1 expects always 0; avoids host PDC churn per integration_spec "rt_safe_swap"). Store prior reported latency in processor. Neural prepare/setModel never changes latency.  
8. Confirm `applyState` / `applyStateToParams` / `loadFactoryPreset` / host recall / A/B / revert paths all read the 4 fields from state and forward `setEngaged` + conditional `setIr`/`setModel` (already present; just verify engages honored and NONE/FLAT are safe).  
9. In `Convolution::setIr`: consider aligning Trim flag to integration_spec recommendation (`Trim::yes` + Normalise::yes) vs current `Trim::no` (assets are pre-trimmed per capture log; keep `Trim::no` + comment if size savings proven, else sync to spec). Test both.  
10. Update `docs/dsp.md` + `docs/architecture.md` (they still describe old File/ONNX/loadIR paths; reflect BinaryData + RTNeural + setIr/setModel + zero-latency + chain order + engage gates).

**P2 — Sync presets exactly to authoritative table + update self-tests (1.5-3h)**  
11. For all 8 voices, set XML drive/mix/variant/cab*/neural*/engages **exactly** per `voice-cab-neural-map.md` table (locked cab/neural + engages; drive/mix may have been QA-nudged since STATUS table). Current XMLs are close but diverge (e.g. VELVET drive 0.5 vs 0.38, CONSOLE GLUE mix 0.85 vs 1.00, SUNDAY DRIVE drive 0.38 vs 0.55).  
12. Extend `runFactoryEmbedTests` (WonKnobberState.cpp): for TAPE HEAD + FURNACE (and ideally all 8), construct full `WonKnobberState` matching the authoritative map (incl. cabIr/neuralModel/engages), generate expected XML, parse BinaryData XML, assert *all* fields roundtrip exactly (remove the tolerant `xmlMatch` hack once disk XMLs match generated). Print full GENERATED_XML_ for each.  
13. Extend state unit tests + preset transport tests to assert the 4 cab fields on more cases (already partially covered by "all voices load clean" + unknown-id sanitizers). Add explicit test case: load each factory voice → verify exact cabIr/neuralModel/engages per map.  
14. Add to DSP smoke tests (or new static test): for each of 6 cabs + 4 models, set + engage + process a buffer → finite + no crash. (Current only spot-checks FLAT + TAPE.) Verify CONSOLE GLUE (FLAT+off+off) is bit-exact vs a pure-saturation reference path if possible.  
15. Update `runPresetTransportAPITests` "all voices load clean" (already iterates) + isDirty four-field tests to also snapshot/verify the cab/neural values from processor after load.

**P3 — Latency/PDC + RT-safety end-to-end verification (0.5-1h + manual)**  
16. In prepare + after setIr: assert `convolution.getLatency() == 0` (or log it) for all 6 IRs. Same for neural (feedforward = 0 added). Total reported latency only from saturation (os-dependent).  
17. Verify `setLatencySamples` is **never** called from audio thread and **not** churned on every preset (only if value changes).  
18. Confirm no heap/locks/allocs/String in process paths (wrap with ScopedNoDenormals already present; profile in AudioPluginHost if unsure).  
19. Fast preset cycle test (see §4).

**P4 — Rear UI hooks + editor sync (blocked until anchors PNG + flip wiring land; 1h once unblocked)**  
20. Add public setters on `WonKnobberAudioProcessor` (or expose via a small struct) for live rear edits:  
   `void setCabIr(const juce::String& id);`  
   `void setNeuralModel(const juce::String& id);`  
   `void setCabEngaged(bool);`  
   `void setNeuralEngaged(bool);`  
   (Each updates the current* member + calls the corresponding dsp setEngaged/set* + marks loadedVoice baseline appropriately. Reuse logic from applyStateToParams.)  
21. In `WonKnobberAudioProcessorEditor::timerCallback` + transport callbacks: sync any new rear state (if FaceplateView adds rear components) similar to variant/bypass. Call `faceplate.setModified(processorRef.isDirty())` etc.  
22. When rear components (from PREP_REAR_UI_READINESS) are added: the wells/rockers/steppers must read current values (from processor accessors) + write via the new setters + repaint on change. Use display names from anchors/map for *painting labels*, translate via bridge or final canonical to the state strings before calling setter. Rear changes must flip `isDirty()` true (already does via 4-field compare) and support revertToLoadedPreset (already wired for the 4 fields).  
23. Update `isDirty()` / `revertToLoadedPreset()` / `getCurrentState()` / `apply*` if any new fields, but per spec they are append-only.

**P5 — Docs + manifest + final polish (0.5h)**  
24. Update `docs/WON_cab_neural_asset_manifest_2026-06-02.json` "example", "canonical_attr_keys", "preset_voice_map_authoritative" to use exact final IDs + authoritative drive/mix from map.md table. Same for integration_spec.md examples.  
25. Add a short "Post-integration" section or pointer in `docs/voice-cab-neural-map.md` (or new CAPTURED_IDS.md) recording the final strings chosen + date.  
26. Ensure `Resources/IRs/LICENSE_IRS.txt` + `Resources/Models/LICENSE_MODELS.txt` + `THIRD_PARTY_LICENSES.md` mention the exact IR/model attributions (from manifest license_notes).  
27. Run full build + all static self-tests (visible on stdout at dlopen) + auval (if on mac).  
28. Update CLAUDE.md or conventions if any new RT rule surfaced.

Estimated total effort once handoff lands + IDs confirmed: **6-10h** (mostly string sync + test extension + one latency pass + docs). Parallelizable (state + DSP + XMLs + processor).

## 2. Potential Code Change Sites (with line hints from current reads)

- **ID lists + sanitizers + defaults (primary sync target):**  
  `Source/WonKnobberState.h:18-19` (comments + defaults)  
  `Source/WonKnobberState.cpp:22` (cab StringArray), `28` (neural), `47-50` (sanitize), `104-105` (fromVT defaults), `136` (legacy comment), `162-163/171-172/185-186/195-196/200` (unit test literals + asserts)  
  `Source/PluginProcessor.h:106-109` (current* members + cabEngage etc inits)  
  `Source/dsp/Convolution.h:23` (comment)  
  `Source/dsp/Convolution.cpp:16-21` (irDataForId ifs), `109` (test setIr), `120` (test log)  
  `Source/dsp/NeuralModel.h:25` (comment)  
  `Source/dsp/NeuralModel.cpp:30-34` (modelDataForId ifs), `90` (NONE comment), `152/167` (smoke test setModel)

- **Preset XMLs (must exactly match final map table):**  
  All 8 under `Resources/factory_presets/{tape_head,console_glue,furnace,velvet,sunday_drive,tube_warm,diode_bite,transformer}.xml` (single-line <WonKnobberState ... cabIr="..." neuralModel="..." cabEngage="0|1" neuralEngage="0|1"/> + drive/mix per voice table).

- **Processor load/apply + PDC paths (verify + extend):**  
  `Source/PluginProcessor.cpp:36-42` (prepare: setEngaged + setIr/setModel), `179-194` (applyStateToParams: current* = + setEngaged + conditional set*), `164-167` (getCurrentState), `214-221` (isDirty: only 4 fields), `229-233` (revert: only 4 fields), `272-283` (loadFactoryPreset), `32-33` (setLatency only sat today), prepareToPlay full, setStateInformation paths that call applyState.  
  Add: public setters (~after line 89), full latency calc + conditional re-set, perhaps `int getConvolutionLatency() const` wrapper.

- **Factory preset table + transport tests (extend assertions):**  
  `Source/PluginProcessor.cpp:248-257` (kFactoryPresets names + BinaryData symbols — unchanged), `619-634` (all voices load clean loop), `594-617` (isDirty four-field), `509` etc (FURNACE load).

- **State factory embed tests (make strict + cover cab/neural):**  
  `Source/WonKnobberState.cpp:318-352` (TAPE HEAD), `354-386` (FURNACE) — populate full state incl. cab from map, assert cab fields in thisOk, drop tolerance once XMLs synced. Extend to more/all voices.

- **CMake + BinaryData (no change unless filenames change):**  
  `CMakeLists.txt:76-85` (the IRs + Models list — stable because filenames locked in manifest).

- **Docs (must be updated for accuracy):**  
  `docs/dsp.md:12-25`, `docs/architecture.md:7/35-36`, `docs/WON_cab_neural_*` (examples + tables), `docs/voice-cab-neural-map.md` (if IDs finalized differently), integration_spec.md, manifest.json.

- **Editor / Faceplate for rear (future hook sites):**  
  `Source/PluginEditor.cpp:152-198` (timer syncs), `134-146` (revert callback), transport lambdas. Add rear sync + callbacks once FaceplateView gains rear state.  
  `Source/gui/FaceplateView.*` (current has no rear; will host the anchored components + flip state + wells that read/write the 4 fields).

- **Self-test entry points (force run on load):** static bools at bottom of .cpp files above.

No changes needed in Saturation, DryWet, Parameters, most GUI components, or BinaryData symbols.

## 3. Risks

- **ID string finalization is the single point of churn.** If handoff picks display names (spaces + TAPE-1971 etc.) for state/XML, ~20-30 string literals + all 8 XMLs change; dataForId keys change (or need aliasing). If they keep current state_ids, only XML drive/mix/engages + docs + test expectations update. Bridge exists precisely to allow UI (rear) to show pretty names while state uses compact ids. Risk of mismatch between what rear paints vs what gets stored (causes "unknown id → FLAT/NONE sanitize" on roundtrip).  
- **BinaryData symbols are filename-derived and stable** (ir_studio_ribbon_wav etc.). Even if ID strings or manifest content update, no CMake or generated-symbol changes. Safe. (If handoff *renames* .wav/.json files, then full rebuild + symbol updates in the 4 if-ladders + CMake list + manifest.)  
- **Preset XML content changes** (drive/mix nudges or ID form) but filenames + BinaryData symbols unchanged. Self-tests that do tolerant xmlMatch or partial field checks will need tightening.  
- **PDC changes on preset swap** — host can glitch (esp. AU). Mitigation already in code (Latency{0} ctor + trimmed mono IRs) + plan item to gate setLatencySamples. v1 all cabs report 0; if one IR ever needs tail, only that voice bumps PDC (rare, document).  
- **Engage semantics + CONSOLE GLUE bare path.** XMLs + map deliberately set cabEngage=0 + neural=0 for CONSOLE GLUE. Disengage must be *bit-exact* passthrough (tests already assert for the stages individually; full chain test needed).  
- **Rear UI not present yet** — setters + processor API must be added now (even if no UI calls them) so when PREP_REAR_UI_READINESS + anchors PNG land, the next CC PR can wire without touching processor again. Risk: timing of flip/anchors handoff vs this integration.  
- **Drive/mix in map are "placeholders"** — voice-cab-neural-map.md says pending WON QA A/B. Handoff may deliver updated numbers; XMLs + embed test hardcodes + STATUS tables must be overwritten. Cab/neural assignments are "locked".  
- **Trim flag / load args in setIr** — current `Trim::no` vs spec `Trim::yes`. Wrong choice could change effective IR length or latency (though pre-trim + 0-latency ctor protects). Verify post-sync with getCurrentIRSize() or audible test.  
- **No runtime file I/O** — all via BinaryData (already correct).  
- **Static self-tests run on every dlopen** — changing literals can make "PASS" → "FAIL" visible immediately in build logs / AudioPluginHost launch. Good for CI.  
- **Legacy state** — old blobs (pre-cab) always default cab=FLAT / neural=NONE / engages=false (intentional, keeps audio identical). Tests cover this.

## 4. Test Strategy

**Static / compile-time (mandatory on every build):**  
- All existing: WonKnobberState units (roundtrip, missing→default, unknown→sanitize, legacy), factory embed (TAPE HEAD/FURNACE + extended), V1 legacy blobs, Convolution smoke (passthrough + finite + unknown), Neural smoke (same), Preset transport API (n=8, names, load, AB, slots, randomize, undo, isDirty four-field rule, "all voices load clean" loop).  
- After: extend factory embed + DSP smokes to cover *all* 6+4+8 combinations explicitly. Assert exact cab/neural values post-load from XML match map. Make embed tests strict (exact XML string match after sync).

**Runtime / functional (in AudioPluginHost or host):**  
- Build Debug + load VST3/AU. Open 8 factory voices in order + random order + fast < > cycling.  
  - Confirm: no clicks/pops/zipper/dropouts on cab+neural swaps (even with cabEngage/neuralEngage flipping).  
  - Meters move, gem changes, drive/mix change per voice.  
  - CONSOLE GLUE sounds "bare saturator" (A/B against a voice with cab/neural on).  
  - Rear (when present): flipping rockers + stepping wells instantly changes sound without click; modified dot lights; revert clears only the 4 fields.  
- True bypass (global): bit-exact dry passthrough (already in processor); I/O meters still run.  
- Bit-exact when cab+neural disengaged (or NONE/FLAT engaged): capture render of CONSOLE GLUE vs a drive-only path; diff should be near-zero (within float eps + any sat dither).  
- PDC: in host that reports latency, confirm stable across all 8 voices (no jumps). Offline render (32x) should align with realtime.  
- Host recall + A/B + modified-dot + revert roundtrips the 4 fields (already partially tested in preset API).  
- Legacy state load (if you have old .vst3 state blob) still sounds identical (no cab/neural coloration added).

**No-click / RT-safety proof:**  
- Fast preset cycle while monitoring CPU + watching for xruns (in a host that shows them).  
- If clicks appear on cab swap: the qa_fallback in manifest is "add 32-128 sample crossfade between conv engines" (follow-up PR; do not implement in this handoff unless explicitly asked).  
- Profile: no allocations in processBlock during voice changes (Instruments or simple cout of new/malloc not firing).  
- Use `juce::dsp::Convolution::getLatency()` + own getCurrentIRSize() in a debug build to assert 0 across swaps.

**Optional harness (future):** If a RenderHarness or offline render test appears, feed it the 8 voices + assert output checksums or finite + energy deltas between voices (FLAT/NONE should be closest to pure sat).

**CI / auval:** Must stay green. Self-test couts must all say PASS.

**Manual verification checklist (post-build):**  
1. Load, cycle all presets rapidly → no audio artifacts.  
2. Toggle bypass → meters keep moving, audio dry.  
3. Load CONSOLE GLUE → cab/neural stages truly bypassed (use a scope or A/B with a "drive only" hack).  
4. With rear (later): set cabEngage off on a voice that has it on → sound changes cleanly to bare; dot lights; revert restores.  
5. Save session, reopen → cab/neural per last voice (or dirty state) restored.

## 5. What the Main Agent Should Ask the Human or CC on Handoff

- "Confirm final canonical strings for cabIr / neuralModel in WonKnobberState / XML / isKnown* / DSP maps: exactly the display names from voice-cab-neural-map.md (FLAT / STUDIO RIBBON / VINTAGE 4x12 / ... + NONE / TAPE-1971 / VALVE-CLASS A / TRANSISTOR-FET / IRON-TRANSFORMER) **or** the current state_ids from bridge/manifest (FLAT / STUDIO_RIBBON / VINTAGE_4X12 / ... + NONE / TAPE / VALVE / TRANSISTOR / IRON)? Provide the exact 11 strings if any tweak."  
- "Drive/mix values in the voice table are still the authoritative targets? Or deliver the post-QA diffs now (we will overwrite the 8 XMLs + embed test expectations + any STATUS tables)?"  
- "Any change to IR/model filenames or BinaryData list? (Plan assumes filenames locked; only content or ID *strings* may reconcile.)"  
- "Rear anchors PNG + flip-spec final? (We can land the processor public setters + editor sync hooks in this PR even if painting waits; confirm order.)"  
- "Should state store the display names (human-readable XMLs) or keep compact ids (current) with bridge for UI? (Bridge already maps design↔state_id; recommend keeping internal ids unless CC explicitly wants display in persisted state.)"  
- "Any other updates to manifest, capture log, or rear-panel-anchors.json that affect strings or per-voice assignments?"  
- "Do we need to surface the active cab/neural in About or a status readout now, or wait for rear panel?"

Copy the exact voice table + bridge table into the handoff reply for CC to ACK.

## 6. Prep Diffs / Sed-Ready ID Maps (generated now)

Two regimes. Use the one matching the handoff decision. All maps are case-sensitive exact.

### Current (state_id / manifest "id" — what code + XMLs use today)
Cab (6):  
FLAT, STUDIO_RIBBON, VINTAGE_4X12, CONSOLE_BOX, OLD_RADIO, IRON_CORE  

Neural (5):  
NONE, TAPE, VALVE, TRANSISTOR, IRON  

### Display / Design-locked (per voice-cab-neural-map.md table + anchors + flip notes)
Cab (6):  
FLAT, "STUDIO RIBBON", "VINTAGE 4x12", "CONSOLE BOX", "OLD RADIO", "IRON CORE"  
(Note: map uses ASCII "x" not ×; anchors use "4x12"; some manifest display use "4×12" — normalize on final.)

Neural (5):  
NONE, "TAPE-1971", "VALVE-CLASS A", "TRANSISTOR-FET", "IRON-TRANSFORMER"

### Bridge (display → state_id, for UI paint vs storage)
From cab-neural-id-bridge.json:
- FLAT ↔ FLAT
- STUDIO RIBBON ↔ STUDIO_RIBBON
- VINTAGE 4x12 ↔ VINTAGE_4X12
- CONSOLE BOX ↔ CONSOLE_BOX
- OLD RADIO ↔ OLD_RADIO
- IRON CORE ↔ IRON_CORE
- NONE ↔ NONE
- TAPE-1971 ↔ TAPE
- VALVE-CLASS A ↔ VALVE
- TRANSISTOR-FET ↔ TRANSISTOR
- IRON-TRANSFORMER ↔ IRON

### Authoritative per-voice assignments (use for XMLs + tests; from voice-cab-neural-map.md)
(Use final ID spelling for cabIr/neuralModel columns.)

| # | name | drive | mix | cabIr (final spelling) | neuralModel (final) | cabEngage | neuralEngage |
|---|------|-------|-----|------------------------|---------------------|-----------|--------------|
| 0 | TAPE HEAD | 0.42 | 1.00 | STUDIO RIBBON | TAPE-1971 | true | true |
| 1 | CONSOLE GLUE | 0.30 | 1.00 | FLAT | NONE | false | false |
| 2 | FURNACE | 0.86 | 1.00 | VINTAGE 4x12 | VALVE-CLASS A | true | true |
| 3 | VELVET | 0.38 | 0.90 | STUDIO RIBBON | TAPE-1971 | true | true |
| 4 | SUNDAY DRIVE | 0.55 | 0.85 | OLD RADIO | TRANSISTOR-FET | true | true |
| 5 | TUBE WARM | 0.48 | 1.00 | CONSOLE BOX | VALVE-CLASS A | true | true |
| 6 | DIODE BITE | 0.70 | 0.95 | VINTAGE 4x12 | TRANSISTOR-FET | true | true |
| 7 | TRANSFORMER | 0.60 | 1.00 | IRON CORE | IRON-TRANSFORMER | true | true |

### Current XML values (for diffing / sed)
(Extracted verbatim; note drive/mix deltas from table above.)
- tape_head.xml: drive="0.42" mix="1.0" ... cabIr="STUDIO_RIBBON" neuralModel="TAPE" cabEngage="1" neuralEngage="1"
- console_glue.xml: drive="0.3" mix="0.85" ... cabIr="FLAT" neuralModel="NONE" cabEngage="0" neuralEngage="0"
- furnace.xml: drive="0.86" mix="1.0" ... cabIr="VINTAGE_4X12" neuralModel="VALVE" cabEngage="1" neuralEngage="1"
- velvet.xml: drive="0.5" mix="0.7" ... cabIr="STUDIO_RIBBON" neuralModel="TAPE" cabEngage="1" neuralEngage="1"
- sunday_drive.xml: drive="0.38" mix="0.9" ... cabIr="OLD_RADIO" neuralModel="TRANSISTOR" cabEngage="1" neuralEngage="1"
- tube_warm.xml: drive="0.6" mix="1.0" ... cabIr="CONSOLE_BOX" neuralModel="VALVE" cabEngage="1" neuralEngage="1"
- diode_bite.xml: drive="0.72" mix="0.8" ... cabIr="VINTAGE_4X12" neuralModel="TRANSISTOR" cabEngage="1" neuralEngage="1"
- transformer.xml: drive="0.55" mix="0.95" ... cabIr="IRON_CORE" neuralModel="IRON" cabEngage="1" neuralEngage="1"

### Suggested sed / replace patterns (run after deciding final IDs; backup first)
Example (switching *to* display names; reverse for the other way; do not run blindly):

```sh
# Cab (state files + tests + comments)
find Source -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i '' \
  -e 's/STUDIO_RIBBON/STUDIO RIBBON/g' \
  -e 's/VINTAGE_4X12/VINTAGE 4x12/g' \
  -e 's/CONSOLE_BOX/CONSOLE BOX/g' \
  -e 's/OLD_RADIO/OLD RADIO/g' \
  -e 's/IRON_CORE/IRON CORE/g' {} +

# Neural
find Source -type f \( -name "*.h" -o -name "*.cpp" \) -exec sed -i '' \
  -e 's/"TAPE"/"TAPE-1971"/g' \
  -e 's/"VALVE"/"VALVE-CLASS A"/g' \
  -e 's/"TRANSISTOR"/"TRANSISTOR-FET"/g' \
  -e 's/"IRON"/"IRON-TRANSFORMER"/g' {} +

# Then hand-edit the 8 XMLs + any StringArray literals that now need spaces + the dataForId ifs + all test literals + defaults.
# Also update every comment that lists the enum.
```

For XML batch (safer to edit by hand or script that also sets the drive/mix per table):

```sh
# Example for one; repeat with correct final spelling + numbers from map table
# sed -i '' 's|drive="0.5" mix="0.7" variant="amethyst" bypass="0" cabIr="STUDIO_RIBBON" neuralModel="TAPE" cabEngage="1" neuralEngage="1"|drive="0.38" mix="0.90" variant="amethyst" bypass="0" cabIr="STUDIO RIBBON" neuralModel="TAPE-1971" cabEngage="1" neuralEngage="1"|' Resources/factory_presets/velvet.xml
```

After sed, **manually**:
- Fix the isKnown* StringArrays and the if (id == "...") ladders in irDataForId / modelDataForId (they must list the final strings exactly, and map to correct BinaryData symbols).
- Update all test hardcoded states and assertions.
- Regenerate any "GENERATED_XML_..." expectations in test output.
- Re-run cmake + build + inspect stdout for all PASS.

Also update the 4 places that mention the enum in comments (headers + sanitize comments).

### Quick audit command (run on arrival to find remaining sites)
```sh
grep -n -E 'FLAT|STUDIO_RIBBON|VINTAGE_4X12|CONSOLE_BOX|OLD_RADIO|IRON_CORE| TAPE |VALVE|TRANSISTOR[^-]|IRON[" ]' \
  Source/WonKnobberState.* Source/PluginProcessor.* Source/dsp/Convolution.* Source/dsp/NeuralModel.* \
  Resources/factory_presets/*.xml docs/*.md docs/*.json 2>/dev/null | cat
```

This + the checklist above is sufficient to execute the handoff PR in one shot.

---

**End of PREP_INTEGRATION_PLAN.md.** Ready for handoff message → immediate implementation. All evidence from exhaustive read/grep of docs/ + Source/ + Resources/ + CMake + JUCE headers in build/. No assumptions beyond the provided context.
