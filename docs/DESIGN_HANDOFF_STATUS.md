# Design handoff — filesystem status (2026-06-02 pm)

| File | In repo? | Notes |
|------|----------|-------|
| `voice-cab-neural-map.md` | Yes (#43) | Authoritative voice map |
| `SYNC_TO_CC_cab_neural_direction.md` | Yes | §1–§4 rear-fold direction |
| `rear-panel-anchors.json` | Yes (**#51** on `main`) | Re-measured coords — use this for PR4 |
| `modified-from-preset-indicator.md` | Yes | 4-field dirty rule (PR #50) |
| `preset-override-precedence.md` | Yes | Stamp-six / persist-until-load |
| `rear-panel-bg.html` | Yes | Source mock; baked PNGs in `Resources/` |
| `rear-panel-background-*.png` | **PR #52** | Rose/amber re-bake from Design zip `(2)` 2026-06-02 |
| `bypass-dimstate-tokens.json` | Yes (#46) | Design-canonical |
| `cab-neural-id-bridge.json` | Yes (#43) | UI display ↔ `state_id` |

## Still owed from Design (PR4 blockers)

- **Flip spec / flip-trio** — not present in the Jun 2 design-system zip; corner-screw → rear rotation wiring waits on this.
- `CURRENT_VERSIONS.md` inside the zip is stale (claims anchors uncommitted; #51 landed).

**Recurrence fix:** Design workspace → Kali path drop → Grok PR (`repo-handoff/` only) → CC merges → PR4 paint.

## 2026-06-03 Design-gate broadcast (ingested by Grok)

- File ingested (same as flip-spec): `repo-handoff/docs/SYNC_FROM_DESIGN_2026-06-03.md` (source) → `docs/SYNC_FROM_DESIGN_2026-06-03.md` + mirror at worktree/project root for quick ref.
- Covers:
  - Web fixes shipped.
  - Grok's two unblocked ingest items: **flip-spec** (already done: `docs/flip-spec.html` from rear-panel-bg.html) + **CW filmstrips** (carbon weave / carbon_fiber_weave assets now cleared for ingest/generation).
  - CC's knob-rotation parity + the single-render-pass note.
  - Two calls waiting on Kali: rear steel-vs-rose + the pixel-check screenshot.
- Tell the team: full broadcast at `repo-handoff/docs/SYNC_FROM_DESIGN_2026-06-03.md` (mirror at project root). This unblocks CW filmstrip work (copy carbon_fiber_weave texture + any rendered *_carbon_* or update render for carbon material filmstrips into sprites/ with locked names; update CMake BinaryData if new assets added).
- Next Grok actions pending this + Kali calls: ingest CW filmstrips (overwrite/add to sprites/ or Resources/), ensure they match the single-render-pass + rotation parity notes from CC.

### Rear/flip UI prep (from fanned general-purpose subagent)
- Subagent 019e8cd2-eb15-73e3-b4f1-4e6ed294c262 succeeded (203s, 61 calls); produced `PREP_REAR_UI_READINESS.md` (committed).
- Full analysis: flip-spec.html (detailed HTML prototype with CABINET/NEURAL modules, rockers, wells, flip medallion using Hultog/Orbitron/amber/rose-gold; exact opts lists matching manifest IDs), rear-panel-anchors.json (all [x,y,w,h] + _notes for live vs baked).
- Current gaps (evidence): No rear/flip in PluginEditor/FaceplateView (front-only 960x600, inline bypass/preset only; no isRear, no rear bg load, no processor setters for cab/neural exposed). Rear PNGs in Resources/ but unembedded until this prep. Processor has internal currentCabIr etc + DSP wiring ready.
- Mapping + components: Table of anchors -> EngageRocker / CabSelectWell / NeuralSelectWell / RearPanelView (or inline like front). Exact state IDs to use (FLAT etc, TAPE etc). Display names for painting from HTML/bridge.
- Build: Quick win executed — added rear-panel-background-*.png (960 + 2x) to CMakeLists.txt BinaryData (now committed). Fonts (Hultog/CF) still owed. Update Resources/README done.
- Suggested order in the report (Grok: embed/processor API/ID maps; CC: scaffold views; shared: editor flip + wiring).
- Ties to handoff: Unblocks CC's rear-panel-ui (PR#53 wk-rear-ui worktree); flip-spec now in docs/ + anchors; rear steel-vs-rose call on Kali noted.
- All PREP reports (incl this + ID/plan/QA) + SYNC committed on branch for handoff execution.

### Integration plan subagent (plan: 019e8cd2-a93a-7900-a408-04cf7f748a99)
- Subagent completed successfully (229s, 78 calls); produced detailed `PREP_INTEGRATION_PLAN.md` (already committed from fanout).
- Prioritized checklist: P0 ID string reconciliation (state_id vs display from voice-cab-neural-map.md), P1 wiring/PDC/latency (Trim flag, full conv latency in prepare + conditional on swap), P2 preset XML sync to authoritative table + strict self-tests, P3 latency verification, P4 rear hooks (public setters — **executed as quick-win above**), P5 docs.
- Critical sites: WonKnobberState.cpp (isKnown*, sanitize, tests), DSP dataForId tables, all 8 factory XMLs, PluginProcessor apply/prepare/isDirty/revert + preset tests.
- Test strategy: extend static smokes + factory embeds + preset transport "all voices" + isDirty asserts; runtime fast-cycle no-click test in host; bit-exact for disengaged.
- Risks: ID churn is main (BinaryData symbols stable since filenames locked); PDC host glitches; CONSOLE GLUE bare path must stay exact.
- Questions for handoff (in plan §5): final ID spelling (display or compact?), drive/mix updates?, rear timing.
- Prep diffs/sed maps included in the MD (current vs display).
- As prep: added the public setters (see commit 76e8135) so rear can wire without re-touching processor.
- Status: plan ready to turn into PR once handoff confirms final IDs + any drive/mix tweaks from map.

### ID audit subagent (explore: 019e8cd2-8940-7030-a356-b224fe8d0aa3)
- Subagent completed successfully (344.9s, 71 calls); produced detailed `PREP_ID_AUDIT.md` (already committed in fanout 0e17f07; content confirmed/ authoritative from this run).
- Exhaustive table of ~100+ ID occurrences (absolute paths + :line) across code (WonKnobberState isKnown*/sanitize/tests, PluginProcessor currents/apply/isDirty, DSP dataForId, XMLs, CMake, manifest, bridge, map.md, anchors, flip-spec, THIRD_PARTY, etc.).
- Core drift: **state form** (FLAT/STUDIO_RIBBON/VINTAGE_4X12/... + TAPE/VALVE/... short; used in code/XMLs/manifest preset_voice_map/bridge state_id/sanitize) vs **display form** (spaces/"VINTAGE 4x12"/full "TAPE-1971"/"VALVE-CLASS A" etc. in map.md table + "Canonical attribute keys" + rear UI notes/anchors/flip/THIRD_PARTY).
- Drive/mix: only TAPE HEAD + FURNACE match map exactly; 6/8 XMLs have shipped §1 values (map pending QA A/B nudges).
- Load path: fully traced (loadFactoryPreset → BinaryData XML → fromValueTree → applyState → applyStateToParams → DSP setEngaged/conditional setIr/setModel; prepare re-applies; A/B uses full state + applyToParams; isDirty/revert only 4 cab/neural fields; loadedVoice snapshot on full apply). Matches integration_spec exactly.
- Gaps/mismatches (vs spec/manifest/briefing):
  - Trim::no (code Conv.cpp:65, "pre-trimmed") vs spec Trim::yes.
  - Latency: only sat at prepare (PluginProcessor:33); no conv.getLatency() sum or swap-conditional update (v1 zero-lat safe but incomplete per spec pdc + "recompute").
  - Stale: processor.h:73 "six identity fields" (but 4-field rule in isDirty/tests/design docs); dsp.md outdated (pre-cab-neural, loadIR/ONNX).
  - Manifest: "neural_slots" (no "neural_assets"); production.trim_taps but load Trim::no.
  - Factory embed tests (state.cpp): partial on cab/neural (only drive/mix/var/bypass for 2 voices; tolerant xmlMatch; comment "after we sync disk xml").
  - Engage: consistent (CONSOLE GLUE 0/0 bare; others 1/1).
  - BinaryData/filenames: exact match to manifest (stable).
- Exact handoff edit list:
  - Grok (docs/Resources): 8 XMLs (sync drive/mix from map table; keep or update cab/neural per final canonical); voice-cab-neural-map.md (reconcile "canonical keys" + table to document state_id for attrs/XML vs separate UI display col; align casing); manifest.json (sync display_names/casing, add neural_assets alias?); cab-neural-id-bridge.json (minor casing); THIRD_PARTY (if neural names change); rear-panel-anchors.json + flip-spec.html (update option lists if IDs change); integration_spec.md (example + Trim note + map).
  - CC (Source/): WonKnobberState (isKnown*/defaults/sanitize/tests), PluginProcessor (currents/apply/isDirty/revert/preset tests/comments), dsp/Conv+Neural (dataForId tables + smokes + comments); rebuild/re-embed; fix latency/Trim if decided.
- Readiness: 65/100 (strong on state forms + load path + tests + assets + RT-safe core; main deltas drive/mix XMLs + doc drift + minor rt wiring). If keep state forms as canonical for attrs (update map to reflect), ~85/100. If force display into state, ~30/100 (big churn).
- Recommendations (from audit): treat bridge + manifest state_ids + current XMLs as de-facto canonical for cabIr/neuralModel attrs (update map.md claims/table); sync XML drive/mix post-QA; fix Trim + add full PDC (sum + conditional); expand embed tests to full cab/neural + all voices; update stale docs/comments.
- All PREPs now complete (this audit + plan + rear + manual QA) + committed; fanout index + status updated for all 4. Ready for handoff "update IDs + re-embed presets" (use the edit list + maps in the MDs).

**Direct "tell" file created for Claude Code + Claude Design:** `docs/GROK_ACCOMPLISHMENTS_2026-06-03.md` (committed explicitly in 2d85371). Comprehensive summary addressed to each:
- For CC: full list of subagent preps (with ids, what each produced, edit lists/readiness from ID audit + plan + QA + rear), quick wins (rear embed + public API), records, next steps for you (use the PREPs for your PR stack).
- For Design: broadcast ingest, unblocked items actioned (flip-spec analyzed, CW sources ready), subagent analysis of your flip-spec.html + anchors (gaps, mapping, order), quick wins that unblock you (embed + API), recs for your next drops (final anchors PNG, CW renders, rear steel-vs-rose + pixel-check for Kali).
- Includes branch state, explicit-path commits, readiness for handoff execution.
This + the updated DESIGN_HANDOFF_STATUS.md + the ingested broadcast itself = the full communication to the team. (Mirror at root if needed for quick ref.)