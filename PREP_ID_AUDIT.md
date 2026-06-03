# PREP_ID_AUDIT.md — CabIr / NeuralModel ID Audit (wk-grok-assets @ feat/grok-asset-sync)

**Date:** 2026-06-03 (exploration only; read-only tools used throughout)  
**Scope:** Full audit of cabIr / neuralModel / cabEngage / neuralEngage ID references + cross-ref to authoritative sources in this worktree.  
**Mission prep for handoff:** Identify mismatches, load path, gaps, files needing edit on handoff (IDs + re-embed presets), readiness.

**Key authoritative sources (this tree):**
- `docs/voice-cab-neural-map.md` (Claude Design; "supersedes"; canonical per-voice table + "Canonical attribute keys (LOCKED)")
- `docs/WON_cab_neural_asset_manifest_2026-06-02.json` (Grok; includes ir_assets, neural_slots, preset_voice_map_authoritative using state_ids, schema_v2, cmake list)
- `docs/cab-neural-id-bridge.json` (explicit design/display ↔ state_id mapping + factory_voice_preset_values using state_ids)
- `docs/WON_cab_neural_integration_spec_2026-06-02.md` (load path, rt_safe rules, example XML using state form)
- Current XMLs in `Resources/factory_presets/` (the baked values)
- Code (for current impl IDs)

**ID forms (drift source):**
- **State / code / XML / manifest ids (used in WonKnobberState, DSP lookups, BinaryData, XML attrs, preset_voice_map_authoritative):** `FLAT`, `STUDIO_RIBBON`, `VINTAGE_4X12`, `CONSOLE_BOX`, `OLD_RADIO`, `IRON_CORE` (cab); `NONE`, `TAPE`, `VALVE`, `TRANSISTOR`, `IRON` (neural, short).
- **Design / display / UI (voice-map table "Cab IR"/"Neural" cols, anchors notes, flip-spec, THIRD_PARTY models list, bridge "design" col):** `FLAT`, `STUDIO RIBBON`, `VINTAGE 4x12` (space, lower x), `CONSOLE BOX`, `OLD RADIO`, `IRON CORE`; `NONE`, `TAPE-1971`, `VALVE-CLASS A`, `TRANSISTOR-FET`, `IRON-TRANSFORMER` (full).
- Bridge.json + manifest's "preset_voice_map_authoritative" + XMLs + code are aligned on **state form**. voice-cab-neural-map.md table + its "canonical keys" section + rear UI notes use **display form**. This is the documented drift (per user briefing).

**Engage defaults (consistent):** Struct/legacy/from-missing = `false` (cabEngage/neuralEngage). CONSOLE GLUE deliberately `0/0` (bare reference voice); all other 7 voices `1/1` (per map note + XMLs + bridge + manifest).

**Drive/mix vs map table:** Only TAPE HEAD + FURNACE match map.md exactly. Others in XMLs are earlier/"shipped §1" values; map lists proposed (pending QA A/B). Manifest's preset_voice_map_authoritative omits drive/mix (only cab/neural/engages).

**BinaryData symbols vs manifest:** Exact match. CMakeLists.txt (lines 76-85) + manifest "cmake_binary_data_append" + "binary_data_symbol" fields + code lookups (ir_*.wav → `ir_*_wav`; model_*.json → `model_*_json`) align. Filenames in Resources/ match manifest.

## Full Load Path Trace (confirmed in code + spec §4 + §5)
1. `loadFactoryPreset(int i)` (PluginProcessor.cpp:272)
   - Index into `kFactoryPresets[]` (names + `BinaryData::xxx_xml` / Size; order matches CMake + getNum/getName).
   - `xmlText = String::fromUTF8(BinaryData...)`
   - `auto xml = XmlDocument::parse(xmlText)`
   - `vt = ValueTree::fromXml(*xml)`
   - `st = WonKnobberState::fromValueTree(vt)`
   - `applyState(st)`
2. `WonKnobberState::fromValueTree` (WonKnobberState.cpp:69)
   - Walks for "WonKnobberState" child (handles WONKNOBBER root + slots).
   - `getProperty("cabIr", "FLAT")`, `getProperty("neuralModel", "NONE")`, `getProperty("cabEngage", false)`, `getProperty("neuralEngage", false)`.
   - Then `sanitizeState(s)` (calls isKnownCabIr / isKnownNeural; unknown → FLAT/NONE; engages untouched).
3. `applyState(const WonKnobberState& st)` (PluginProcessor.cpp:197)
   - `applyStateToParams(st)`
   - `slotA = slotB = st; activeSlot = 'A'; loadedVoice = st;` (seats baseline for isDirty() + A/B)
4. `applyStateToParams` (PluginProcessor.cpp:171)
   - Copy fields to `currentCabIr`, `currentNeuralModel`, `cabEngage`, `neuralEngage`.
   - `convolution.setEngaged(cabEngage); if (cabEngage) convolution.setIr(currentCabIr);`
   - `neuralModel.setEngaged(neuralEngage); if (neuralEngage) neuralModel.setModel(currentNeuralModel);`
5. DSP side (message thread):
   - Convolution::setIr (Convolution.cpp:53): dedupe on currentIrId; `irDataForId(id, data, size)` → `loadImpulseResponse(data, size, Stereo::no, Trim::no, 0, Normalise::yes)` (note Trim vs spec); update currentIrId.
   - NeuralModel::setModel (NeuralModel.cpp:79): dedupe; if !modelDataForId (NONE/unknown → reset inactive to nullptr + flip); else parse nlohmann json + RTNeural::json_parser::parseJson into inactive, atomic flip activeIdx.
6. prepareToPlay (PluginProcessor.cpp:26; also called on rate/block change):
   - sat.prepare + initial `setLatencySamples(sat only)`
   - conv.prepare + `setEngaged` + conditional `setIr`
   - neural.prepare + `setEngaged` + conditional `setModel`
7. Other paths:
   - Ctor: `applyState(getCurrentState())` (defaults).
   - setStateInformation (host recall): magic WK2 + XML path → fromValueTree → applyState; or legacyV1 → applyState (cab fields get struct defaults).
   - A/B (setActiveSlot etc.): `applyStateToParams` (saves/loads full state incl. cab/neural to slots; does **not** touch loadedVoice).
   - isDirty() / revertToLoadedPreset(): compare/revert **only the 4 cab/neural identity fields** (design rule; drive/mix/variant/bypass ignored for "modified" dot). loadedVoice re-seated only on full applyState (factory load, recall).
   - Rear overrides (future): write to live state + saveToActiveSlot (full state); A/B slots carry cab/neural.

**Self-tests exercising path/IDs (static init, visible on dlopen):**
- WonKnobberState: roundtrip nominal (VINTAGE_4X12/IRON + engages), missing→defaults (FLAT/NONE/false), unknown→safe defaults, factory embed (partial; only checks drive/mix/var/bypass for TAPEHEAD/FURNACE; xmlMatch tolerant).
- PluginProcessor preset transport: num=8, names, load FURNACE (drive/var), AB switch/save (full state), copy, save/load/randomize, undo, isDirty four-field (drive ride stays clean), "all voices load clean" (critical: every factory XML roundtrips to !isDirty() via loadedVoice), revert.
- Convolution smoke: disengaged passthrough, engaged FLAT finite, unknown "NOT_A_CAB" no-op.
- Neural smoke: disengaged passthrough, engaged TAPE finite, setModel("NONE") no-op.
- All pass in current tree (build exists).

## Table of All ID Occurrences (file:line : value used)
(Compiled exhaustively via list_dir + repeated read_file + grep on Source/ + Resources/ + docs/ + CMake + THIRD_PARTY; absolute paths. Build/ deps and juce internals excluded. Lines from read_file outputs or grep hits. "display" = human/UI form; "state" = code/XML form.)

**Core code (state + processor + DSP lookups):**
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.h:18 : cabIr{"FLAT"} (state default + comment lists all state IDs)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.h:19 : neuralModel{"NONE"} (state default + comment lists all state IDs)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:22 : isKnownCabIr: {"FLAT", "STUDIO_RIBBON", "VINTAGE_4X12", "CONSOLE_BOX", "OLD_RADIO", "IRON_CORE"}
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:28 : isKnownNeural: {"NONE", "TAPE", "VALVE", "TRANSISTOR", "IRON"}
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:48 : sanitize → s.cabIr = "FLAT"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:50 : sanitize → s.neuralModel = "NONE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:62 : vt.setProperty("cabIr", cabIr...
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:63 : vt.setProperty("neuralModel", neuralModel...
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:64-65 : cabEngage / neuralEngage props
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:104 : s.cabIr = ...getProperty("cabIr", "FLAT")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:105 : s.neuralModel = ...getProperty("neuralModel", "NONE")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:106-107 : cabEngage/neuralEngage defaults false
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:136 : legacy comment: struct defaults (FLAT/NONE, both disengaged)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:162 : test: orig.cabIr = "VINTAGE_4X12"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:163 : test: orig.neuralModel = "IRON"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:164-165 : test engages true
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:171-172 : back asserts "VINTAGE_4X12" / "IRON" / engages
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:185-186 : missing defaults assert "FLAT" / "NONE" / false
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:195-196 : bad test "NOT_A_CAB" / "NOT_A_MODEL"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.cpp:200 : ok assert back "FLAT" / "NONE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.h:73 : comment (stale): six identity fields (drive/mix/cabIr/...)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.h:106 : currentCabIr{"FLAT"}
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.h:107 : currentNeuralModel{"NONE"}
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.h:108-109 : cabEngage{false}; neuralEngage{false}
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:36-38 : prepare: setEngaged(cabEngage); if() setIr(currentCabIr)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:40-42 : prepare: setEngaged(neuralEngage); if() setModel(currentNeuralModel)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:164-167 : getCurrentState: copies currentCabIr etc + engages
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:179-182 : applyStateToParams: current* = st.* ; cabEngage= ; neuralEngage=
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:186-188 : apply: conv setEngaged + if setIr(currentCabIr)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:192-194 : apply: neural setEngaged + if setModel(currentNeuralModel)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:214 : isDirty: if (live.cabIr != loadedVoice.cabIr)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:216 : isDirty: neuralModel
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:218-220 : isDirty: cabEngage / neuralEngage (only these 4; drive/mix excluded)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:230-233 : revertToLoadedPreset: copies only the 4 cab/neural from loadedVoice
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:249-256 : kFactoryPresets (display names only; values inside XMLs)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:503 : test names "TAPE HEAD" / "CONSOLE GLUE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:599 : test loadFactoryPreset(0) // TAPE HEAD
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.cpp:606 : test load(2) // FURNACE
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.h:23 : comment: manifest slot id (FLAT/STUDIO_RIBBON/VINTAGE_4X12/CONSOLE_BOX/OLD_RADIO/IRON_CORE)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:16-21 : irDataForId ifs: "FLAT", "STUDIO_RIBBON", "VINTAGE_4X12", "CONSOLE_BOX", "OLD_RADIO", "IRON_CORE" (exact state form)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:55 : setIr dedupe
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:60 : if (! irDataForId...)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:68 : currentIrId =
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:109 : test conv.setIr ("FLAT")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:125 : test "NOT_A_CAB"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.cpp:120 : test log "engaged FLAT finite"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.h:25 : comment: manifest slot id (NONE/TAPE/VALVE/TRANSISTOR/IRON)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.h:27 : comment: NONE / unknown...
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.cpp:30-33 : modelDataForId ifs: "TAPE", "VALVE", "TRANSISTOR", "IRON" (NONE falls to return false)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.cpp:88 : if (! modelDataForId...)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.cpp:90 : comment // NONE/unknown → passthrough
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.cpp:152 : test nm.setModel ("TAPE")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.cpp:163 : test log "engaged TAPE finite"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.cpp:167 : test nm.setModel ("NONE")

**Factory preset XMLs (Resources/; single-line root elements; all use state-form IDs + engages as 0/1 strings):**
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/tape_head.xml:3 : cabIr="STUDIO_RIBBON" neuralModel="TAPE" cabEngage="1" neuralEngage="1" (drive="0.42" mix="1.0" variant="diamond" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/console_glue.xml:3 : cabIr="FLAT" neuralModel="NONE" cabEngage="0" neuralEngage="0" (drive="0.3" mix="0.85" variant="onyx" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/furnace.xml:3 : cabIr="VINTAGE_4X12" neuralModel="VALVE" cabEngage="1" neuralEngage="1" (drive="0.86" mix="1.0" variant="ruby" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/velvet.xml:3 : cabIr="STUDIO_RIBBON" neuralModel="TAPE" cabEngage="1" neuralEngage="1" (drive="0.5" mix="0.7" variant="amethyst" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/sunday_drive.xml:3 : cabIr="OLD_RADIO" neuralModel="TRANSISTOR" cabEngage="1" neuralEngage="1" (drive="0.38" mix="0.9" variant="citrine" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/tube_warm.xml:3 : cabIr="CONSOLE_BOX" neuralModel="VALVE" cabEngage="1" neuralEngage="1" (drive="0.6" mix="1.0" variant="citrine" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/diode_bite.xml:3 : cabIr="VINTAGE_4X12" neuralModel="TRANSISTOR" cabEngage="1" neuralEngage="1" (drive="0.72" mix="0.8" variant="emerald" bypass="0")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/transformer.xml:3 : cabIr="IRON_CORE" neuralModel="IRON" cabEngage="1" neuralEngage="1" (drive="0.55" mix="0.95" variant="sapphire" bypass="0")

**CMake (file paths only; symbols derived):**
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/CMakeLists.txt:76-81 : IRs paths (ir_flat.wav → ir_flat_wav etc.)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/CMakeLists.txt:82-85 : Models paths (model_tape.json → model_tape_json etc.)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/CMakeLists.txt:68-75 : factory_presets XML paths (for BinaryData)

**Docs / manifest / bridge / spec (cross-ref sources):**
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/voice-cab-neural-map.md:18 : cabIr values (display form): `FLAT` / `STUDIO RIBBON` / `VINTAGE 4x12` / `CONSOLE BOX` / `OLD RADIO` / `IRON CORE`
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/voice-cab-neural-map.md:19 : neuralModel values (display form): `NONE` / `TAPE-1971` / `VALVE-CLASS A` / `TRANSISTOR-FET` / `IRON-TRANSFORMER`
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/voice-cab-neural-map.md:28-35 : table (display forms + drive/mix/gem/engages per voice; e.g. row0: STUDIO RIBBON | TAPE-1971 | on | on ; row1: FLAT | NONE | off | off)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:22 : ir_assets[0].id: "FLAT" (state)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:33 : "STUDIO_RIBBON"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:44 : "VINTAGE_4X12"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:55 : "CONSOLE_BOX"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:66 : "OLD_RADIO"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:77 : "IRON_CORE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:95 : neural_slots[0].id: "NONE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:105 : "TAPE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:115 : "VALVE"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:125 : "TRANSISTOR"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:135 : "IRON"
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:164-171 : preset_voice_map_authoritative.voices[] (state form + engages; matches current XMLs exactly for cab/neural/engages; e.g. "cabIr": "STUDIO_RIBBON", "neuralModel": "TAPE"...)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:182 : schema: cabIr enum (state form with _ )
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:183 : neuralModel enum (short state)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:187 : example XML attr (state form: cabIr="VINTAGE_4X12" neuralModel="IRON")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_integration_spec_2026-06-02.md:110 : example: cabIr="STUDIO_RIBBON" neuralModel="TAPE" ...
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_integration_spec_2026-06-02.md:116 : neuralModel enum (short)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/cab-neural-id-bridge.json:6-11 : cab_ir[] (design display with spaces/4x12 vs state_id _/4X12)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/cab-neural-id-bridge.json:14-18 : neural_model[] (design full like "TAPE-1971" vs state_id short "TAPE")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/cab-neural-id-bridge.json:23-30 : factory_voice_preset_values.voices[] (state form cabIr/neuralModel + engages; exact match to XMLs + manifest preset_voice_map)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/rear-panel-anchors.json:15 : _cab_ir_well_note (display): FLAT / STUDIO RIBBON / VINTAGE 4x12 / CONSOLE BOX / OLD RADIO / IRON CORE
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/rear-panel-anchors.json:21 : _neural_model_well_note (display): NONE / TAPE-1971 / VALVE-CLASS A / TRANSISTOR-FET / IRON-TRANSFORMER
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/flip-spec.html:233-242 : cab options (display, unicode ×): VINTAGE 4×12, FLAT, STUDIO RIBBON, CONSOLE BOX, OLD RADIO, IRON CORE
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/flip-spec.html:275-283 : neural options (display): TAPE — 1971, NONE, VALVE — CLASS A, TRANSISTOR — FET, IRON — TRANSFORMER
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/SYNC_TO_CC_cab_neural_direction.md:111 : `cabIR` (old casing; display proposal)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/THIRD_PARTY_LICENSES.md:48 : RTNeural models list (display full): (TAPE-1971, VALVE-CLASS A, TRANSISTOR-FET, IRON-TRANSFORMER)
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json:25,35,... : display_name fields (e.g. "STUDIO RIBBON", "VINTAGE 4×12" note ×, "TAPE")
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/voice-cab-neural-map.md:23 : note on neuralModel key (and manifest must use it)
- Multiple docs (preset-override-precedence.md:7,10; modified-from-preset-indicator.md:18; etc.): generic references to the four fields (no literal ID values).

**Other (no literal state/display IDs):**
- Scripts/* : filenames only (ir_*.wav, model_*.json); voice names in comments.
- Resources/README.md, IRs/LICENSE_IRS.txt, Models/LICENSE_MODELS.txt : generic.
- GUI (FaceplateView.*, PluginEditor.*, other gui/): no literal cabIr/neural ID strings (only generic comments + preset display names + dirty logic via processor.isDirty()).
- PluginEditor.cpp: only loadFactoryPreset calls + name sync (no IDs).

## Mismatches vs Authoritative Map / Manifest / Bridge / Spec / XMLs
1. **ID form drift (core issue):**
   - voice-cab-neural-map.md:18-19 + table cols: display forms (spaces, "VINTAGE 4x12", full "TAPE-1971" etc.).
   - But bridge.json + manifest preset_voice_map_authoritative + all 8 XMLs + isKnown* + irDataForId/modelDataForId + defaults + tests + schema example: state form ( _ , UPPER 4X12, short TAPE/VALVE...).
   - map.md "Canonical attribute keys" claims "All three sides (CC WonKnobberState, Grok manifest, preset XMLs, rear-panel-anchors.json bindings) use exactly these keys" — but they use state form; anchors use display.
   - rear-panel-anchors.json + flip-spec.html + THIRD_PARTY: display (for UI painting / notices).
   - bridge.json exists precisely to translate (design → state_id for state/XML/BinaryData).

2. **Drive/mix values in XMLs vs voice-cab-neural-map.md table (map is "authoritative 8-voice"):**
   - tape_head + furnace: exact match (0.42/1.0 diamond; 0.86/1.0 ruby).
   - console_glue: drive 0.3≈0.30 ok, but mix="0.85" vs map 1.00.
   - velvet: "0.5"/"0.7" vs 0.38/0.90.
   - sunday_drive: "0.38"/"0.9" vs 0.55/0.85.
   - tube_warm: "0.6" vs 0.48, mix ok.
   - diode_bite: "0.72" vs 0.70, "0.8" vs 0.95.
   - transformer: "0.55" vs 0.60, "0.95" vs 1.00.
   - Manifest's preset_voice_map_authoritative lists **no** drive/mix (only cab/neural/engages + names); XMLs are the "current baked".
   - Map note: "Drive/mix are placeholders pending the WON QA A/B-by-ear pass".

3. **Engage in console_glue:** XML 0/0 + map off/off (correct; only bare voice). All others 1/1.

4. **Casing/display variants:**
   - map table: "VINTAGE 4x12" (lower x); bridge design: "VINTAGE 4x12"; anchors: "VINTAGE 4x12"; flip: "VINTAGE 4×12" (×); state/XML: "VINTAGE_4X12".
   - Neural display in map/anchors/flip/THIRD_PARTY: "TAPE-1971" / "TAPE — 1971" etc.; state: "TAPE"; manifest neural_slots display_name="TAPE" (short).

5. **Manifest structure gaps (per briefing example):**
   - No "neural_assets" section (uses "neural_slots" + "ir_assets").
   - "preset_voice_map_authoritative" uses state_ids (correct for CC); "ui_display_names" points to cab-neural-id-bridge.json.
   - In manifest ir_assets: "display_name" sometimes uses × ("VINTAGE 4×12").

6. **Spec vs code (rt_safe / load / latency):**
   - integration_spec: loadImpulseResponse(..., Trim::yes, ...). Code (Convolution.cpp:65): Trim::no (comment: "pre-trimmed in the asset pipeline"). Manifest/scripts do trim to 2048/1536 taps.
   - Spec pdc: "setLatencySamples(saturation + convolution.getLatency() + neuralLatency) at prepare; on IR swap only re-report if changed (v1 expects 0)".
     Code: only `setLatencySamples(saturation.getLatencySamples())` at prepare (PluginProcessor:33); no conv.getLatency() or neural; no update in setIr path or applyStateToParams. (Conv ctor uses Latency{0}; neural=0 always; v1 safe but incomplete.)
   - Spec load path example matches code exactly (incl. messageThreadAsync note for assets).
   - Conv load in code: Trim::no + Normalise::yes + 0 (full); spec says Trim::yes.
   - In prepareToPlay: conv/neural setIr/setModel happen on message (good); but sat latency set before conv.prepare.

7. **Stale/outdated comments/docs:**
   - PluginProcessor.h:73: "six identity fields (drive/mix/...)" but impl + design docs (modified-from-preset-indicator.md, preset-override-precedence.md, isDirty tests) use only 4 cab/neural for dirty/revert/dot.
   - dsp.md: outdated (loadIR, ONNX, no fields).
   - Some early docs use `cabIR` (upper R).
   - State factory embed tests (WonKnobberState.cpp:318+): genXml uses struct defaults for cab/neural (FLAT/NONE/0/0); disk XMLs have per-voice; only tolerant xmlMatch; asserts skip cab/neural (comment: "after we sync disk xml").
   - Map.md claims locked canonicals that don't match what sides actually use.

8. **XML vs state defaults in tests:** State embed tests for TAPEHEAD/FURNACE create defSt without overriding cab/neural (so genXml gets defaults), while real XMLs have voice-specific. The "all voices load clean" test in processor relies on XMLs matching exactly what fromValueTree + loadedVoice produces.

9. **No other hardcodes in GUI/Source:** No cab/neural ID literals in editor/faceplate (rear not wired yet; uses processor for state + dirty). Preset strip uses display names + indices only.

10. **Assets vs manifest:** Resources/ IRs/Models filenames + count match manifest (6+4). No extra/ missing. Scripts generate matching trimmed files.

## Exact List of Files That Would Need Edit on Handoff (with Proposed Canonical IDs from the Map)
Handoff context (per briefing + CLAUDE.md): Grok writes ONLY to Resources/IRs/ (locked ir_*.wav), Resources/Models/ (locked model_*.json), THIRD_PARTY_LICENSES.md, docs/. CC owns Source/ edits. "handoff lands: update IDs + re-embed presets" implies: (a) sync drive/mix + any ID canonicals into the 8 XMLs (re-embed via BinaryData), (b) reconcile doc drift in docs/, (c) possibly update manifest/bridge if IDs change, (d) CC will then sync Source/ (isKnown*, irDataForId, modelDataForId, defaults, tests, comments, apply paths) + re-run static tests + re-embed. If "canonical from the map" means adopt display forms into state (unlikely—breaks BinaryData? no, but UI vs state separation via bridge is intentional), or (more likely) update map.md table/claims to use state forms for attrs while keeping display cols for humans.

**Must-edit on Grok side (docs + Resources presets + THIRD_PARTY):**
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Resources/factory_presets/*.xml (all 8): Update drive/mix to exact map.md table values (e.g. console_glue mix="1.0"; velvet drive="0.38" mix="0.90"; sunday_drive drive="0.55" mix="0.85"; etc.). Keep cabIr/neuralModel/engages in current state form (or change if map canonicals win). Re-embed required after.
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/voice-cab-neural-map.md: Reconcile "Canonical attribute keys" section (lines 18-21) + table (28-35) to consistently document state_ids for cabIr/neuralModel values (use bridge design for "UI display" column); update note on locked sides; align VINTAGE casing/x and neural short vs full. Or adopt display as "canonical" (would require XML + CC state change).
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_asset_manifest_2026-06-02.json: Add explicit "neural_assets" alias or rename neural_slots for symmetry? Sync any drive/mix if added to preset_voice_map_authoritative; ensure display_name in ir_assets match map (e.g. "VINTAGE 4x12"); add cross-ref note to bridge for design names.
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/cab-neural-id-bridge.json: Minor: ensure "VINTAGE 4x12" casing consistent; add drive/mix if they become locked post-QA.
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/THIRD_PARTY_LICENSES.md: (if IDs change) update the models list in RTNeural section (line 48) to match final canonical (currently uses display full names).
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/rear-panel-anchors.json + docs/flip-spec.html: (if final IDs change) update the _note strings listing options (display forms).
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/WON_cab_neural_integration_spec_2026-06-02.md: Update example (110) + proposed map (old §6) + any Trim::yes note if code changes; align with final canonical.

**CC-side files that would need edit post-handoff (Source/; listed for completeness per task, even though Grok does not touch):**
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/WonKnobberState.h + .cpp (isKnown*, defaults, sanitize, fromValueTree getProperty defaults, toValueTree, all unit tests with literals like "VINTAGE_4X12", legacy comments).
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/PluginProcessor.h + .cpp (current* inits, prepare/applyState*/getCurrentState/isDirty/revert, comments, preset tests, kFactoryPresets names if voice order changes).
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/Convolution.h + .cpp (irDataForId ifs + comments + smoke tests "FLAT").
- /Users/kalimeeks/Documents/GitHub/wk-grok-assets/Source/dsp/NeuralModel.h + .cpp (modelDataForId ifs + comments + smoke tests "TAPE"/"NONE").
- Rebuild + re-embed presets (CMake BinaryData will pick updated XMLs); update any stale comments (e.g. PluginProcessor.h:73).
- If canonical changes to display forms: also update XML attrs in the 8 presets + all string compares/lookups.

**No-edit (or low risk):** CMake (already has files), Scripts (filenames), GUI (no literals), BinaryData symbols (stable on filenames), Resources/IRs/Models files themselves (locked names).

## Other Gaps Found
- **Latency/PDC incomplete:** As noted in mismatches. Spec requires sum + conditional re-set on swap; code only does sat at init. (Safe for v1 zero-lat but violates "trace the full" + future-proofing.)
- **Trim flag mismatch:** Code Trim::no vs spec Trim::yes (and integration example).
- **Factory embed tests incomplete for cab/neural:** State.cpp tests don't assert cab/neural in TAPEHEAD/FURNACE XML roundtrips (only drive/mix/var/bypass); xmlMatch is or-tolerant. Processor's "all voices load clean" is the stronger coverage.
- **Stale header comment vs design:** isDirty "six fields" in .h but four-field rule everywhere else (including tests + design md files).
- **Manifest vs briefing expectation:** "neural_assets" absent (neural_slots present); ir_assets has production.trim_taps but load uses Trim::no.
- **No rear wiring yet:** No ID usage in GUI (expected; rear-panel-ui blocked on Design PNG/anchors per spec + DESIGN_HANDOFF_STATUS.md). cab-neural-id-bridge + rear-panel-anchors + flip-spec are present for CC PR4.
- **Legacy + missing keys:** Handled (defaults to FLAT/NONE/false); good for upgrade path.
- **A/B + dirty + overrides:** Fully wired for the 4 fields + full state in slots (per preset-override-precedence.md + modified-from-preset-indicator.md). loadedVoice snapshot + isDirty only on 4 is correct.
- **Static self-tests:** Comprehensive + required (run on load); cover unknown IDs, roundtrips, factory XML parse, all-voices clean, AB with state, dirty four-field.
- **No heap/RT issues in ID paths:** Lookups are static arrays + BinaryData (pre-embedded); loads on msg thread; atomics for engaged/idx.
- **Other docs gaps:** dsp.md / architecture outdated (pre-cab-neural); some SYNC/PLAN use old `cabIR`.
- **Voice order / index:** Consistent (0=TAPE HEAD ... 7=TRANSFORMER) across kFactoryPresets, XML filenames, map table, manifest preset_voice_map, bridge, processor tests.
- **No "neural" key drift in current code:** All "neuralModel" (old spike/docs had "neural").

## Readiness Score for "handoff lands: update IDs + re-embed presets"
**Score: 65/100** (moderate-high; IDs mostly stable in the "state" form that code/XML/manifest/bridge use; main work is drive/mix sync + doc reconciliation).

- **Strengths (high readiness):** Code + XMLs + manifest preset_voice_map + bridge factory values + isKnown*/DataForId all perfectly aligned on state IDs + engages (console off/off, others on/on). Load path fully traced + matches spec. Self-tests cover ID paths + "all voices clean". BinaryData symbols/filenames match manifest. A/B/dirty/revert/legacy/prepare paths handle the 4 fields. Assets present + embedded in CMake. RT-safe (msg thread loads, atomics, no alloc in process) per spec (minor Trim/latency gaps).
- **Blockers / work on handoff (reduces score):** Drive/mix drift in 6/8 XMLs vs authoritative map table (must update + re-embed). ID form drift between map.md "canonical" (display) vs actual used (state) — requires doc fix in map.md + possibly manifest; bridge is the translator but map claims "LOCKED" values sides don't use. Latency/Trim mismatches vs spec (CC side + possible conv load change). Stale comments in Source/headers. Incomplete factory embed asserts for cab/neural. "neural_assets" naming in manifest. Outdated ancillary docs (dsp.md).
- **Post-handoff CC effort:** Update ~5 Source files (state/processor/dsp) for any ID canonical decision + comments + latency wiring + test asserts; re-embed; verify all static tests + no clicks on preset cycle (per qa_fallback in manifest).
- **If no ID change (keep state form as "canonical" for attrs, display only in UI notes):** Score rises to ~85/100 (only drive/mix XML updates + doc polish + latency fix).
- **If map display forms become canonical for cabIr/neuralModel values:** Score drops to ~30/100 (massive edit to all state strings, XMLs, tests, lookups; bridge becomes identity; UI lists change; risk to serialization stability).

**Recommendations for handoff:**
- Treat bridge.json + manifest preset_voice_map_authoritative + current XML state-form IDs as the de-facto canonical for attrs/XML/code (update map.md "canonical keys" + table to document "state_id (for XML/state/BinaryData)" vs separate "UI display name").
- Sync the 8 XML drive/mix from map table (or get final QA numbers).
- Fix Trim:: flag + add latency sum + swap notification (even if 0 for v1).
- Expand state factory embed tests to assert full cab/neural per XML.
- Update stale comments + dsp.md.
- After Grok XML/doc updates + re-embed in this tree, CC can cherry-pick or handoff the Resources/ + docs/ and stack Source changes.
- Re-run full build (zero warnings), AudioPluginHost load of all 8 voices, sanity (no zipper on cycle, meter moves, dirty dot only on cab/neural override, A/B preserves cab/neural), static test logs clean.

**End of audit.** All exploration via read_file / list_dir / grep only. No source edits performed. PREP_ID_AUDIT.md written to cwd as required. Ready for handoff review.