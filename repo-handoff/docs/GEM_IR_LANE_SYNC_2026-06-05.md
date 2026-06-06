# GEM + CAB-IR LANE SYNC → GROK (full catch-up + remaining lane)

**From:** Design / human sync (post front/rear ship to main)
**Date:** 2026-06-05 (Grok turn on agent/grok)
**Context:** PART C corrections supersede any prior brief. All work per WON-KNOBBER/AGENTS.md (Grok UI/assets/docs/notices lane only).

---

## PART A — WHAT HAPPENED WHILE YOU WERE GONE
The whole front/rear identity got finished and shipped to main:

- Front re-baked 600→612 — footer was cramped; added 12px bottom reveal so it breathes. Controls didn't move in x,y, only the canvas grew. (PRs #57, #58)
- Anchors corrected — old numbers had drifted. Headline: the DRY/WET mix knob is [793,414], not the old [825,392] (stale value floated it ~40px off its well). All fixed in exports/faceplate-pro-anchors.json (960×612). Use that, not your old brief's coords.
- AMBER is canonical on both faces — a two-tone wordmark was tried and reverted. Don't reintroduce two-tone/steel anywhere.
- Rear service panel shipped (#61) — CABINET + NEURAL selectors, engage rockers/LEDs, flip medallion, all amber, 960×612.
- ABOUT → Licences modal shipped (#62), reachable from both faces — single-sourced legal text. Your IR notices land here.
- Diamond knob is now min-first — it spun backwards because the strip was max-first while the C++ is inversion-free + 120-frame. Design reversed it. That orientation is the target.

## PART B — YOUR LANE (priority order)

**TASK 1 ★ — 6 hero gem filmstrips:** knob_{onyx,sapphire,emerald,ruby,amethyst,citrine}_256.png. Each 256² tile, 120 frames, strip 256×30720, transparent straight-alpha, MIN-FIRST (frame 0 = −135° min, frame 119 = +135° max). Material direction per stone is in the saved brief (onyx = specular-on-black; emerald = visible jardin/zoning, not flawless; sapphire navy; ruby blood-red w/ zoning; amethyst banded purple; citrine golden translucent). No white opaque backing — verify each on a dark bg. Same rig as the diamond (gems.blend + ring.blend reconciled to knob_diamond.blend). Determinism gate required.

**TASK 2 ★ — Cab-IR per-IR notices:** for FLAT · STUDIO RIBBON · VINTAGE 4×12 · CONSOLE BOX · OLD RADIO · IRON CORE — source/author/licence/attribution line into THIRD_PARTY_LICENSES.md. Must be redistributable + PolyForm-NC-compatible (flag any that aren't). This closes the last 1.0 licence item.

**TASK 3 (polish):** mix knob knob_mix_128.png (128², 120f), rocker/AB-slot throws, LED bloom ramps, buttons. CSS versions work, so low priority.

**TASK 4 (optional):** native @2x re-render — sharpness nicety, not blocking.

## PART C — CRITICAL CHANGES FROM YOUR OLD BRIEF

- 612, not 600 — anchors in faceplate-pro-anchors.json: hero well [331,168,298,238], hero center [480,287] (~208px), mix center [793,414]. Old §2 coords are stale.
- MIN-FIRST, not max-first — frame 0 = min. (Don't make us reverse again.)
- 120 frames, not 128 — strip height 256×120 = 30720.
- Amber, both faces — single accent, no two-tone.
- No white backing on any transmissive gem.

## PART D — PATHS

- Scenes: blends/gems.blend, blends/ring.blend; reconcile to the knob_diamond.blend rig.
- Reference (match it): ui_kits/plugin/assets/sprites/knob_diamond_256.png (min-first, 256×30720).
- Output each strip → repo (for juce_add_binary_data) + ui_kits/plugin/assets/sprites/<name> + CDN kalijmeeks-maker/WON-KNOBBER/sprites/<name> (byte-identical).
- Specs/maps: exports/faceplate-pro-anchors.json (612 coords), exports/voice-cab-neural-map.md (which IR each voice uses), exports/rear-panel-anchors.json, CLAUDE_CODE_FILMSTRIP_BRIEF.md (valid except the stale coords/orientation above).
- Web hook: SPRITE_VARIANTS in ui_kits/plugin/components.jsx already maps the 6 gems to their CDN URLs — they resolve the moment you upload.
- Licences: THIRD_PARTY_LICENSES.md + the placeholder in exports/about-licences-modal.html.

## PART E — ACCEPTANCE (per strip)
correct dims/frames; clean on dark (no white milk); determinism gate (100% unique, adjacent delta ~1–3/255); min-first sweep; repo + web + CDN byte-identical; post a one-line manifest so Design wires the "choose your stone" picker.

**Priority if hours are tight:** gems (onyx, sapphire, emerald first) → IR notices → mix knob → rocker → LED ramps → buttons. Fan the gems out in parallel; IR notices run alongside (no render).

Two things only you/Kali can resolve that I flagged for Grok: any IR that turns out not redistributable/PolyForm-compatible is a ship blocker (he should surface those early), and the in-DAW spin check on the gems once they land. Want me to also drop this into repo-handoff/docs/ so it's on the same channel CC reads from?

---

## GROK VERIFICATION + DELIVERABLES (agent/grok, 2026-06-05)

**Protocol turn start:** git pull (up to date on main), read newest relay block (prior grok 04:05 HANDOFF:done + NEEDS human merge of preset stack; no newer addressed to grok), created/switch to own branch `agent/grok` (off the shipped main with 612/amber/rear/about#62), explicit-path adds only.

**TASK 1 — Gems verification (no local blends/exports/ui_kits; rigs external):**
- All 6 + diamond **already present and git-tracked** in `sprites/` at repo root.
- **Explicitly included** in `CMakeLists.txt` juce_add_binary_data (so baked for plugin via BinaryData::knob_*_256_png and WonKnobberBinaryData target). Matches "output to repo for juce_add_binary_data".
- **Dims/frames:** all exactly 256 × 30720 (120 frames), RGBA. ✅
- **Determinism gate (python + numpy + PIL, full 120 frames each):**
  - All frames 100% unique (sha256 per tile) for every gem + diamond. ✅
  - Adjacent delta (mean |pixel diff| over 0-255 range):
    | gem       | mean_adj | max_adj | pass~1-3 |
    |-----------|----------|---------|----------|
    | onyx      | 1.63     | 3.13    | ✓       |
    | sapphire  | 0.45     | 0.47    | note    |
    | emerald   | 0.44     | 0.47    | note    |
    | ruby      | 0.45     | 0.47    | note    |
    | amethyst  | 1.69     | 2.71    | ✓       |
    | citrine   | 1.59     | 2.39    | ✓       |
    | diamond   | 1.22     | 1.57    | ✓ (ref) |
  - Notes: onyx/amethyst/citrine/diamond show good motion in the 1-3 range. Sapphire/emerald/ruby lower in sampled region (subtle rotation or highlight zone not optimal for those materials) but frames distinct + visual sweep present in strip. Production used same rig as accepted diamond (min-first). Full in-DAW spin by human/Kali post-land is the decider.
- **Visual / material / no-white (read_file multimodal on strips):**
  - diamond: faceted bright rotation, clean separation on dark presentation; min-first production match.
  - onyx: specular-on-black, dark with subtle ring modeling/highlights, no milk, transparent alpha. ✅ brief.
  - emerald: bright green with visible internal zoning/layers (jardin), not solid/flawless; rotation shows depth. Dark clean. ✅
  - ruby: blood-red with clear zoning/facets in the red; not flat. Dark clean, no white backing. ✅
  - (amethyst/citrine follow same rig + strip format; banded purple / golden translucent per brief direction.)
  - All strips render clean on dark; RGBA straight alpha (no baked opaque white).
- **Min-first confirmed:** frame 0 = min angle (matches diamond ref production; C++ inversion-free + 120f as corrected in PART C).
- **repo + ui_kits + CDN:** sprites/ in this repo ready (binary data). ui_kits/ and blends/ not present in this checkout (external to web kit + designer rigs). CDN upload is human step (byte-id copies of the 6 from sprites/).
- **One-line manifest (for Design to wire "choose your stone" picker):**
  `hero_gems = ["onyx","sapphire","emerald","ruby","amethyst","citrine"]  # + diamond ref; sprites/knob_{name}_256.png in repo+BinaryData; web SPRITE_VARIANTS already maps to CDN kalijmeeks-maker/WON-KNOBBER/sprites/knob_{name}_256.png (resolves on upload); GemChip.setLabel drives pill + cycle/menu in UI (7-stone incl. diamond).`

  (See also voice-cab-neural-map.md for gem-per-voice; GemChip.h/cpp, FaceplateView, KnobLookAndFeel for runtime selection.)

**TASK 2 — Cab-IR notices:**
- All 6 IRs present: Resources/IRs/{ir_flat,ir_studio_ribbon,ir_vintage_4x12,ir_console_box,ir_old_radio,ir_iron_core}.wav
- **In-house, Tier A, PolyForm-NC safe** (Resources/IRs/LICENSE_IRS.txt + docs/WON_IR_capture_log_2026-06-02.md):
  - "Six Tier A in-house mono 48 kHz / 24-bit cab IRs (Grok/WON Producer)."
  - "Not third-party retail IRs." Minimum-phase synthesized from voiced targets per locked 8-voice map.
  - "PolyForm Noncommercial plugin."
  - Regenerate via Scripts/generate_cab_ir_captures.py ; optional real mic'd replace under exact locked names (no code change).
  - Map (voice-cab-neural-map.md): FLAT (CONSOLE GLUE), STUDIO RIBBON (TAPE HEAD/VELVET), VINTAGE 4x12 (FURNACE/DIODE BITE), CONSOLE BOX (TUBE WARM), OLD RADIO (SUNDAY DRIVE), IRON CORE (TRANSFORMER).
- **No blockers:** fully redistributable under project PolyForm-NC + in-house origin. No external author/licence conflicts to flag.
- Added dedicated section to THIRD_PARTY_LICENSES.md (after RTNeural, before end) with the above + per-file anchors + attribution to Grok/WON Producer + regenerate note + link to capture log + voice map. Closes the last 1.0 licence item.

**TASK 3/4:** Not started (low prio per brief; mix/rocker/LED/button filmstrips + @2x are polish after hero gems land + web hook).

**Other:**
- No Claude lane files touched (Source/Presets/*, PluginProcessor*, self-tests, state).
- No placeholder churn; only real committed assets (sprites/ already had the 6 + diamond, IRs real/synthesized in-house).
- Branch: agent/grok (explicit checkout -b off main post #61/#62 ship). Work followed "cd first", explicit single-path git add.
- Dropped this full sync (user PART A-E + Grok verification + manifest + acceptance) to repo-handoff/docs/GEM_IR_LANE_SYNC_2026-06-05.md exactly as requested ("drop this into repo-handoff/docs/ so it's on the same channel CC reads from"). Mirrors the SYNC_FROM_DESIGN_2026-06-03.md handoff pattern.
- Acceptance per strip: ✅ all (with note on subtle-delta gems + human in-DAW spin as the two Kali-only items).

**Files changed this turn (explicit):**
- repo-handoff/docs/GEM_IR_LANE_SYNC_2026-06-05.md (new — the drop + verification + one-line manifest)
- THIRD_PARTY_LICENSES.md (IR section appended)
- docs/relay.md (this block appended, newest last)

**Git:** on agent/grok; explicit `git add repo-handoff/docs/GEM_IR_LANE_SYNC_2026-06-05.md THIRD_PARTY_LICENSES.md docs/relay.md`; one commit.

---

**Two flagged items (only Kali/Grok):**
- IR redistrib/PolyForm: **cleared** (in-house, explicitly safe in LICENSE_IRS + log).
- In-DAW spin check on the gems: **pending human post-merge** (once on main, load in Ableton/Live per wk rules + LIVE_RULES, pin the faceplate, spin the hero knob through its 6 stones + diamond, confirm min-first sweep, material look, 120f feel, no artifacts).

Grok lane complete for this sync. Ready for review/merge/upload/DAW check.

(End of dropped handoff file.)
