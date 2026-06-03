# PREP_REAR_UI_READINESS.md

**WON-KNOBBER · General-purpose prep sub-agent output (rear/flip UI + build/docs readiness)**

Date: 2026-06-03 (per workspace). All exploration confined to `/Users/kalimeeks/Documents/GitHub/wk-grok-assets` (current worktree). The separate `wk-rear-ui` worktree (CC PR#53 `feat/rear-panel-ui`) was not modified and paths were not assumed/accessed beyond obvious workspace root.

## 1. Summary of flip-spec.html + rear-panel-anchors.json (controls, layout language)

### flip-spec.html (full read; 310 lines; the HTML prototype for rear service panel)
- **File**: `docs/flip-spec.html`
- **Canvas**: 960×600 (`.chassis`), matching front faceplate ref coords. Dark-mode, rose-gold + near-black aesthetic.
- **Chassis** (matches front `.mk-chassis-rg`):
  ```html
  <div class="chassis" id="chassis">
    <span class="screw tl"></span><span class="screw tr"></span><span class="screw bl"></span><span class="screw br"></span>
  ```
  - Rose-gold polished radial + texture bg, subtle sheen, corner screws (18px, radial-gradient rose-gold with slot highlight; `.screw.tl` etc. at 11px insets).
- **Panel** (`.panel`): recessed dark field (radial near-black + subtle lines), inner shadows, `.sheen` overlay. Matches front `.mk2-panel` + `.pro-panel`.
- **Header** (`.head`): brand SVG (Hultog/CF Engraved "WON" gold-gradient + "KNOBBER" silver), "REAR · SERVICE PANEL" tag, spec-inset (Orbitron "JUCE 8.0.13 · v1.0.0" + "FIRMWARE · BUILD"), head-stamp (SERIAL "WK—0001" amber-engraved).
- **Rule** hairlines (gold/amber gradient, matches front `.mk2-rule`).
- **Body** (`.body` grid 1fr auto 1fr):
  - **LEFT — CABINET module** (`.module` recessed):
    - `<span class="eng md">CABINET</span>` header.
    - Engage row: `.rocker` (64×30, dark gradient cap slider; `.off` state shifts cap left + cools bg) + `.led` (jewel amber 12px; matches front `.pro-stage .mk-led-amber`) + labels "ENGAGE" / "ON" (Orbitron amber).
    - `<span class="eng xs">IMPULSE RESPONSE · CONVOLUTION</span>`
    - `.screen` well (recessed dark; current readout `.cur` Hultog ~17px amber + `◂ ▸` chevrons): e.g. `VINTAGE 4×12`.
    - `.opts` list (engraved 10.5px): 
      ```
      FLAT
      STUDIO RIBBON
      VINTAGE 4×12   (on = .on bright amber-hi + glow)
      CONSOLE BOX
      OLD RADIO
      IRON CORE
      ```
  - **CENTER — FLIP MEDALLION** (hero):
    - "FLIP TO FRONT" (eng md).
    - `.hero-well` + rose-gold `.medallion` (128×128, conic + radial, engraved "↻" + "FLIP").
    - `.hero-readout`: "SAT → CAB → NEURAL" (amber engraved).
    - `.hero-scale`: FRONT / SERVICE / REAR labels.
  - **RIGHT — NEURAL module** (symmetric):
    - `<span class="eng md">NEURAL</span>`
    - Identical engage rocker + LED + "ENGAGE"/"ON".
    - `<span class="eng xs">MODEL · RTNEURAL</span>`
    - Screen + chevrons: e.g. `TAPE — 1971`.
    - `.opts`:
      ```
      NONE
      TAPE — 1971   (on)
      VALVE — CLASS A
      TRANSISTOR — FET
      IRON — TRANSFORMER
      ```
- **Footer** (`.foot`): oversampling segmented (OFF / ×2(on amber) / ×4 / ×8), I/O Trim strip (value + chevrons), foot-btns (ABOUT / MANUAL).
- **Typography/Colors** (CSS vars + classes):
  - `--font-eng:"Hultog Engraved","CF Engraved",serif` (uppercase, bone/amber, letter-spacing, text-shadows).
  - `--font-led:"Orbitron",monospace` (amber glows).
  - Amber: `#ff8800` / `#ffb43c` / `#b25c00`; bone `rgba(238,232,220,.46)`.
  - Rocker/LED/screen/module visuals are explicit (linear-gradients, inset shadows, jewel radial for LED).
- **Purpose** (per task + HTML): "flips" the chassis (affordance on brand/serial? — not implemented in this rear view; medallion is "flip to front"). Exposes cab engage-rocker + IR select-well + neural engage-rocker + model select-well. These **override** per-voice preset values. Front stays pure one-knob + transport/gem. Static labels/recesses baked; live layers (rockers, wells, medallion, segs) painted by code.

### rear-panel-anchors.json (full read)
- **File**: `docs/rear-panel-anchors.json`
- **_space**: "960x600 px, [x,y,w,h] from chassis top-left. Multiply by 2 for the @2x asset."
- **_bg_asset**: `"rear-panel-background-960x600.png (@2x: rear-panel-background-2x-1920x1200.png)"`
- **_note** (key): "Rear panel reworked to match the FRONT faceplate language (rose-gold mk-chassis-rg frame, warm near-black mk2-panel field, pro-module recesses, mk2-rule amber hairlines, jewel LEDs). Layout mirrors the front: header (brand / spec inset / serial stamp) -> rule -> 3-zone body (CABINET / flip medallion / NEURAL) -> rule -> footer (oversampling / I/O trim strip / buttons). The baked PNG is a representative comp (shows VINTAGE 4x12 + TAPE-1971 + engaged). CC paints the live layers below; static labels/recesses are already baked."
- **Live vs static** (direct quotes from notes):
  - `cab_engage_rocker`: [59,168,64,30]; "_cab_engage_rocker_note": "Live. OFF=cap left/cool, ON=cap right/amber."
  - `cab_engage_led`: [134,177,12,12]
  - `cab_ir_well`: [59,232,263,48]; "_cab_ir_well_note": "Live. Paint current IR name (Hultog Engraved amber ~17px) + chevron stepper at right. Options list painted below it: FLAT / STUDIO RIBBON / VINTAGE 4x12 / CONSOLE BOX / OLD RADIO / IRON CORE (active = bright amber)."
  - `neural_engage_rocker`: [638,168,64,30]
  - `neural_engage_led`: [713,177,12,12]
  - `neural_model_well`: [638,232,263,47]; "_neural_model_well_note": "Live. Options: NONE / TAPE-1971 / VALVE-CLASS A / TRANSISTOR-FET / IRON-TRANSFORMER."
  - `flip_medallion`: [416,228,128,128]; "_flip_medallion_note": "Live hit-target. Click rotates chassis 180deg back to front (~450ms cubic-bezier(.2,.8,.2,1)). Static rose-gold disc is baked; only the click + rotation are wired."
  - Others: `spec_inset` static; `oversampling_seg`, `io_trim_strip`, `about_btn`, `manual_btn` live (some future).
- **Full keys** (960x600 chassis origin):
  ```json
  "chassis": [0, 0, 960, 600],
  "spec_inset": [506, 39, 181, 46],
  "cabinet_module": [41, 119, 299, 380],
  "cab_engage_rocker": [59, 168, 64, 30],
  "cab_engage_led": [134, 177, 12, 12],
  "cab_ir_well": [59, 232, 263, 48],
  "neural_module": [620, 119, 299, 380],
  "neural_engage_rocker": [638, 168, 64, 30],
  "neural_engage_led": [713, 177, 12, 12],
  "neural_model_well": [638, 232, 263, 47],
  "flip_hero_well": [386, 198, 188, 188],
  "flip_medallion": [416, 228, 128, 128],
  "oversampling_seg": [41, 543, 160, 28],
  "io_trim_strip": [217, 528, 593, 41],
  "about_btn": [827, 528, 42, 42],
  "manual_btn": [877, 528, 42, 42]
  ```
- **Layout language**: Identical scaling pattern as front (`FaceplateView::place(rx,ry,rw,rh)` using `kRefW=960`, `kRefH=600`, `sx/sy` factors + `roundToInt`). Rose-gold screws/chassis, amber jewels, engraved Hultog for wells, Orbitron for LEDs/headers. Baked PNG provides static recesses/labels; code draws only live affordances + readouts.

**Rear purpose (from specs + HTML + anchors + related docs)**: Overrides for the 8 factory voices' cab/neural (see `cab-neural-id-bridge.json` + `voice-cab-neural-map.md`). Front face pure; rear = service panel for cab IR + neural model + engages. Flip via brand/serial? (front) + center medallion (rear).

## 2. Current code gaps (concrete file + line evidence)

- **No flip affordance or rear state at all**:
  - `Source/PluginEditor.cpp:148`: `setSize(960, 600);` + only `FaceplateView faceplate;`. No `isRear`, no click-to-flip, no dual view. `resized()` just `faceplate.setBounds(...)`.
  - `Source/PluginEditor.h:26`: Sole member is front `FaceplateView`.
  - `Source/gui/FaceplateView.cpp:13`: Hardcodes front: `BinaryData::faceplate_pro_960x600_png`. No rear bg load.
  - No "brand/serial" hit area or screw for flip-to-rear in `FaceplateView::mouseDown` (lines 258-372 only handle bypassRocker, preset chevrons/A/B, transport, about, modified dot, licences).
  - `FaceplateView::paint` (83-107) + `paintOverChildren` (496-518) draw front chassis + overlays only. No conditional rear.

- **No rear FaceplateView variant or RearPanelView**:
  - `Source/gui/` lists (via `list_dir`): `FaceplateView.{h,cpp}`, `BypassLED.*`, `GemChip.*`, `StatusLEDs.*`, `IOMeter.*`, `KnobLookAndFeel.*`, `MixKnob.*`, `TransferCurve.*`, `HarmonicBars.*`, `DbReadout.*`. **Zero rockers, wells, selects, or rear-specific**.
  - Front bypass "rocker" is **inline custom draw** only: `FaceplateView.h:138`: `juce::Rectangle<int> bypassRockerBounds;`, `drawBypassRocker` (cpp:520-559), hit in mouseDown (309-314). No reusable `EngageRocker` component.
  - Preset strip / transport / about are also inline (no extra child comps per style).
  - `FaceplateView.h:98`: "Design reference is the 960x600 PRO chassis" (front only).

- **No IR/model dropdown/select-well components wired**:
  - No code calls `processor.setCabEngage` etc. (see below).
  - `cab-neural-id-bridge.json` (and manifest) exist for mapping, but unused in GUI (only in processor presets + state sanitize).
  - Display names in flip-spec (e.g. "VINTAGE 4×12", "TAPE — 1971") vs state_ids ("VINTAGE_4X12", "TAPE") require explicit map (not present).

- **Processor cab/neural state present but UI-inaccessible (no public setters)**:
  - `Source/PluginProcessor.h:106-109`:
    ```cpp
    juce::String currentCabIr{"FLAT"};
    juce::String currentNeuralModel{"NONE"};
    bool cabEngage{false};
    bool neuralEngage{false};
    ```
  - Internal only via `applyStateToParams` (cpp:179-194), called from ctor/presets/slots/state recall. `prepareToPlay` (36-42) pushes to `convolution.setEngaged`/`setIr` + `neuralModel.setEngaged`/`setModel`.
  - **No public API** like:
    ```cpp
    void setCabEngage(bool e);
    void setCurrentCabIr(const juce::String& id);
    void setNeuralEngage(bool e);
    void setCurrentNeuralModel(const juce::String& id);
    ```
    (grep confirmed; only `getCurrentState`, `isDirty` etc. use them).
  - DSP side ready: `Source/dsp/Convolution.h:23`: `void setIr(const juce::String& cabIrId);` (accepts manifest ids); `setEngaged`. `NeuralModel.h:25,28`: `setModel`, `setEngaged`.
  - `isDirty()` (h:75, cpp:208-222) + `revertToLoadedPreset` (225-235) **already** treat the 4 fields specially (drives front ember dot). Good foundation.
  - Factory presets (cpp:248-257 + `loadFactoryPreset`) use the 8 voices with cab/neural baked (via `WonKnobberState::fromValueTree` + bridge values in XMLs).

- **8-voice cab/neural overrides reflected in UI?** (only via front modified dot + preset loads):
  - **Yes, narrowly**: `FaceplateView` lights ember dot for cab/neural divergence only (see `modified-from-preset-indicator.md` + code).
    - `FaceplateView.cpp:410-432`: `if (modified ...) { draw radial ember + glow at modifiedDotBounds }` (comment: "when the live cab/neural identity diverges from the loaded voice").
    - `h:84-85`: `setModified(processorRef.isDirty())`; `onRevertToPreset` reverts **only** the 4 fields (cpp:134-146 in editor).
    - `isDirty()` docstring (processor.h:72-74): "in ANY of the six identity fields" but impl narrows to 4 cab/neural (cpp:214-221; drive/mix intentionally ignored per design).
    - Gem (variant/stone) **is** per-voice (synced on factory load in editor:61,76,96,...); preset names shown in footer strip (`setPresetDisplayName`).
    - **No direct readout** of current cab/neural on front (by design; hidden until flip). No gem change from rear overrides.
  - Evidence in docs: `preset-override-precedence.md`, `modified-from-preset-indicator.md:16-20` ("only the four identity fields"), `FaceplateView.cpp:212` comment.
  - A/B slots + transport (processor.cpp:291+) already roundtrip the 4 fields.

- **Other gaps**:
  - No rear bg in editor timer sync or paint.
  - No `juce::AudioProcessorValueTreeState` for cab/neural (intentionally state-only like `currentVariant`).
  - Inline drawing style means new rear controls may follow same (or introduce small reusable comps for rocker/well).
  - Fonts: rear prototypes require Hultog/CF Engraved (not in tree yet); front uses inline `Font` for engraved (no BinaryData typeface for them in current front code).
  - Oversampling/I/O trim/About/Manual on rear are sketched in HTML but not core for cab/neural handoff.

## 3. Mapping from anchors keys to likely component bounds or names

Use the same `place()` lambda pattern from `FaceplateView.cpp:115-119` (ref 960×600 → scaled).

| anchors key              | [x,y,w,h] (960x600) | Likely component / usage (in a Rear*View)                  | Notes from _notes + HTML |
|--------------------------|---------------------|------------------------------------------------------------|--------------------------|
| chassis                  | [0,0,960,600]      | Rear root container / bg draw                              | Full panel bounds |
| cabinet_module           | [41,119,299,380]   | Module background (baked or drawn recess)                  | Houses rocker + well + opts |
| cab_engage_rocker        | [59,168,64,30]     | `EngageRocker` or inline draw + hitrect                    | Live; cap pos + .off class; drives cabEngage |
| cab_engage_led           | [134,177,12,12]    | Amber jewel LED (reuse/extend `BypassLED` or `StatusLEDs` style) | Lit when ON |
| cab_ir_well              | [59,232,263,48]    | `CabSelectWell` (screen + chevrons + opts list below)     | Paint Hultog amber name + ◂▸; list of 6 (active bright) |
| neural_module            | [620,119,299,380]  | Symmetric module                                           | |
| neural_engage_rocker     | [638,168,64,30]    | `EngageRocker` (or shared)                                 | |
| neural_engage_led        | [713,177,12,12]    | LED                                                        | |
| neural_model_well        | [638,232,263,47]   | `NeuralSelectWell`                                         | 5 opts (incl NONE); note slight name diffs in HTML |
| flip_hero_well           | [386,198,188,188]  | Container for medallion                                    | |
| flip_medallion           | [416,228,128,128]  | Click hit-target (rose-gold baked in PNG)                  | Triggers 180° flip back to front |
| spec_inset               | [506,39,181,46]    | Static (Orbitron firmware text)                            | Baked |
| oversampling_seg         | [41,543,160,28]    | Segmented control (future?)                                | |
| io_trim_strip            | [217,528,593,41]   | Value + chevrons (future)                                  | |
| about_btn / manual_btn   | [827,528,...]      | Footer buttons (hit targets; open modals)                  | |

- **Component bounds**: In `resized()`, `auto place = ...`; then `cabRocker.setBounds(place(59,168,64,30));` etc. Hit-testing in `mouseDown` (exact rects, like front `bypassRockerBounds`).
- Front precedent: `FaceplateView.cpp:152` (bypassRockerBounds = place(41,531,64,49)); sub-rect calc for chevrons/names (156-188).

## 4. List of new components or extensions needed

Per CLAUDE.md GUI conventions + current style (inline where possible, but task example given):

- **RearPanelView.h/.cpp** (or `RearFaceplateView`): Root container parallel to `FaceplateView`. Loads `BinaryData::rear_panel_background_960x600_png` (and @2x logic if separate). `paint` draws bg scaled. Holds/positions cab/neural modules + flip medallion + footer. Exposes `std::function` callbacks: `onCabEngageToggled`, `onCabIrSelected`, `onNeural...`, `onFlipToFront`. `setCabEngage(bool)`, `setCurrentCabIr(const String& displayOrId)`, etc. for sync. `resized()` does all `place()` + caches hit rects. `mouseDown` for rockers/wells/medallion + chevrons/opts. May draw inline rockers/wells like front bypass/preset (to avoid new files per some guidelines).
- **EngageRocker** (new small Component, reusable for cab + neural): 64×30. Props: `bool engaged`; paints gradient body + sliding cap (left=off cool per HTML `.off`, right=on amber). `onToggled` callback. Mouse cursor hand. (Could be drawn inline in Rear* to match front bypass pattern.)
- **CabSelectWell** (and **NeuralSelectWell**): Or single `SelectWell` templated by list. Bounds from `cab_ir_well`. Paints `.screen` (current name Hultog Engraved amber ~17px + chevrons). Below: vertical `.opts` list (font size ~10.5px). Click on chevron or item cycles/selects; fires callback with state_id. Active item highlighted per `.on` (bright + text-shadow). Needs display-name map.
- **Extensions to existing**:
  - `FaceplateView`: Add front flip affordance (e.g. brand area or serial stamp hitrect + `onFlipToRear` callback). Add `setRear*` no-ops or forward if unified view. Or keep separate.
  - `PluginEditor`: Add `bool isFlipped = false;`; `std::unique_ptr<RearPanelView> rearView;` or two Faceplate-style members. Wire processor getters on init + timer sync (like `setVariant`, `setBypassed`). On flip: hide/show + resize or swap bounds. Handle rotation animation (later; start with instant swap). Add `on*` lambdas that call new processor setters + refresh rear displays.
  - `PluginProcessor`: Add public getters/setters for the 4 fields (e.g. `void setCabEngage(bool); bool getCabEngage() const;`). Inside setter: update member, call `convolution.setEngaged(...)` + `setIr(...)` if on, update dirty/loaded? logic, persist in state. Same for neural (call `neuralModel.*`). Also `juce::String getCurrentCabIr() const;` etc. (for UI init/sync). Expose lists? Or hardcode in UI using bridge.
  - `WonKnobberState`: Already has fields + sanitize (ids locked).
- **Other**:
  - Reusable jewel LED? (small `JewelLED` extending `BypassLED`?).
  - Font loading: Add Hultog + CF to BinaryData + `juce::Typeface` cache (front currently uses generic `Font` for engraved; rear prototypes specify Hultog).
  - ID/display mappers (in util/ or gui/): Use `cab-neural-id-bridge.json` as source of truth for rear wells (design display vs state_id).
  - Modals for ABOUT/MANUAL (reuse/extend current About panel in FaceplateView).
  - (Future) oversampling/I/O trim if in scope.

**Style notes** (from CLAUDE.md + code): 4-space, `juce::` qual, PascalCase classes, camelCase methods, place() scaling, `BinaryData` only (never disk), real-time safe (all UI alloc on msg thread). Run clang-format. Dark only; amber/rose tokens.

## 5. Any ID list for the selects (should match the manifest ids exactly)

**Use manifest / state / sanitize IDs exactly** (underscore, short; from `docs/WON_cab_neural_asset_manifest_2026-06-02.json` + `cab-neural-id-bridge.json` "state_id" + `WonKnobberState.cpp:22,28` + processor defaults + DSP comments). **Never** store display variants in `currentCabIr`/`neuralModel`.

**Cab IR (6)**:
- `"FLAT"`
- `"STUDIO_RIBBON"`
- `"VINTAGE_4X12"`
- `"CONSOLE_BOX"`
- `"OLD_RADIO"`
- `"IRON_CORE"`

**Neural models (5)**:
- `"NONE"`
- `"TAPE"`
- `"VALVE"`
- `"TRANSISTOR"`
- `"IRON"`

**Evidence**:
- `WonKnobberState.cpp:22`: `static const juce::StringArray ids{"FLAT", "STUDIO_RIBBON", ...};`
- `Neural ids:22`: `{"NONE", "TAPE", "VALVE", "TRANSISTOR", "IRON"}`
- Bridge `state_id` column + factory_voice_preset_values use exactly these (e.g. "VINTAGE_4X12", "TAPE").
- Manifest `ir_assets[].id` / `neural_slots[].id`.
- Sanitize falls back to FLAT/NONE.
- Display names (for painting wells/opts) come from bridge "design" or HTML (e.g. "VINTAGE 4×12" / "TAPE-1971" / "VALVE-CLASS A" / "TRANSISTOR-FET" / "IRON-TRANSFORMER"; "STUDIO RIBBON" etc.). UI code must map id ↔ display for `cur` readout + `.opts .on`.

**In rear wells**: Paint using display; on select/click, convert to state_id and call `processor.setCurrentCabIr("VINTAGE_4X12")`.

## 6. Build/docs notes

- **Rear bg embedding update required** (not yet in BinaryData):
  - `CMakeLists.txt:58-86` (`juce_add_binary_data(WonKnobberBinaryData SOURCES ...`): Includes front `faceplate_pro_960x600.png`, Orbitron, 7 knob sprites, 8 XMLs, 6 IR .wav, 4 model .json.
  - **Missing**: `Resources/rear-panel-background-960x600.png` + `rear-panel-background-2x-1920x1200.png`.
  - Add lines (after faceplate):
    ```cmake
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/rear-panel-background-960x600.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/rear-panel-background-2x-1920x1200.png"
    ```
  - Then in code: `BinaryData::rear_panel_background_960x600_png` (name derived from filename; check generated `JuceLibraryCode/` post-CMake).
  - @2x: anchors note says multiply coords by 2; likely load the 960 one and let JUCE/Image handle, or conditional on scale (front currently ignores @2x, uses 960 ref).
  - `Resources/README.md`: Update table to note rear-panel-background-*.png (top-level, not under Faceplates/). Current README assumes subdirs + no binaries committed.
  - Rebuild: `cmake -B build ... && cmake --build build` (must be zero warnings).

- **Fonts for rear**:
  - Prototypes (`flip-spec.html:7-8`, `rear-panel-bg.html`) `@font-face` Hultog_Engraved.ttf + CF_Engraved.ttf (relative `../fonts/`).
  - Current `Resources/Fonts/`: Only `Orbitron.ttf` (embedded + used in spec-inset/LEDs).
  - `README.md:30,81` + `gui.md:23` document them as expected ("CF Engraved & Hultog Engraved (DaFont)").
  - **Action for handoff**: When engraved fonts land in `Resources/Fonts/`, add to `juce_add_binary_data`:
    ```cmake
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/Hultog_Engraved.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/CF_Engraved.ttf"
    ```
  - Load in rear view: `juce::Typeface::createSystemTypefaceFor(BinaryData::Hultog_Engraved_ttf, ...)` or `FontOptions` with typeface. Front engraved text currently bypasses (uses system Bold for "engraved" look); rear wells specify Hultog per anchors note.
  - Add to `.gitignore`? No, they will be committed like Orbitron? (Assets are in repo per current state.)

- **Other build**:
  - `juce_add_binary_data` change requires clean + reconfigure (BinaryData target rebuilt).
  - No change to RTNeural/JUCE fetch or plugin formats.
  - CI (implied by CLAUDE): must pass after embed (assets increase bundle ~few hundred KB; within 350KB budget note in manifest).
  - No `tools/RenderHarness.cpp` present (searched root + Source + Scripts; only JUCE build artifacts reference "headless"). CLAUDE.md "from map" — planned for future headless rear renders (e.g. to validate PNG vs live). Scripts/ has only IR/model generators.
  - Factory XMLs (in Resources/factory_presets/*.xml) already contain cab/neural per bridge (verified via processor load tests).

- **Docs updates needed on handoff**:
  - `docs/gui.md`: Add rear section (anchors usage, Hultog loading, flip affordance, select-well patterns).
  - `CLAUDE.md` File-by-File: Add `RearPanelView.*` under gui/.
  - `docs/rear-panel-anchors.json` + `flip-spec.html` are source-of-truth (do not edit without Design).
  - `Resources/README.md` + top-level asset notes.
  - Possibly new `docs/rear-ui.md` or extend.

## 7. Prep items for handoff

- **"once final anchors land, CC will scaffold; Grok may need to ensure BinaryData for any new rear art or update manifest if new assets"** (verbatim from task + matches `PLAN_BROADCAST_2026-06-02.md`, `DESIGN_HANDOFF_STATUS.md`, `WON_cab_neural_asset_manifest_2026-06-02.json:10`).
  - Rear bg PNGs **already in** `Resources/` (960 + 2x) — Grok's prior ingest; just missing CMake embed.
  - If Design delivers final flip-trio (corner-screw affordance spec) or updated anchors (per DESIGN_HANDOFF_STATUS: "Flip spec / flip-trio — not present"), re-audit coords + update place() calls.
  - Manifest pointer + ids are locked (use bridge for UI); if new assets, append to CMake BinaryData + `isKnown*` arrays + sanitize + DSP load paths + factory XMLs.
  - Grok-owned: assets + CMake + processor public API + ID maps + BinaryData symbols in code.
  - CC-owned (per plans): scaffold RearPanelView + wiring + PR#53.

- **Current readiness**:
  - Assets (6IR + 4 models + rear PNGs) + BinaryData wiring (partial) + state schema (v2 with 4 fields) + DSP setters + isDirty/revert + preset roundtrips: **ready**.
  - GUI: **gaps** (no rear view, no flip, no rockers/wells, no processor UI API, fonts not embedded, no ID<->display in GUI).
  - From `DESIGN_REPO_HANDOFF_README.md`, `PLAN_BROADCAST_2026-06-02_assignments.md:25`: CC blocked on anchors (now present) + will paint live layers.
  - `prep/SUBAGENTS_FANOUT.md` context matches this output exactly.

- **Suggested quick wins before handoff arrival** (Grok can do now, read-only otherwise):
  - Add rear PNGs to CMakeLists.txt BinaryData (low risk; enables later load).
  - Add public accessors + mutators on `WonKnobberAudioProcessor` for cab/neural (with DSP side-effects + state consistency). Add unit-test coverage in the static self-tests section.
  - Extract a tiny `EngageRocker` or jewel LED from inline bypass code (reusable).
  - Stub `RearPanelView.h` skeleton (empty paint/resized) + include in editor compile test.
  - Document exact display<->id map in a small `util/CabNeuralIDs.h` (from bridge.json).

## 8. Suggested order of work when handoff arrives

1. **Embed + build** (Grok): Add rear PNGs (and fonts when present) to `CMakeLists.txt` `juce_add_binary_data`. Reconfigure/build. Verify symbols in `build/.../BinaryData.h`. Update `Resources/README.md`. (Zero warnings required.)
2. **Processor API** (Grok): Add + implement `setCabEngage(bool)`, `getCabEngage() const`, `setCurrentCabIr(const juce::String&)`, `getCurrentCabIr() const`, + neural equivalents. Inside: update members, `convolution.set*` / `neuralModel.set*` (message thread), ensure `getCurrentState` / slots / dirty see it. Add to `applyState*` paths if needed. Extend self-tests. Call from presets already works.
3. **ID/display helpers** (Grok): Small header (or in util/) with `StringArray` for lists + `toDisplayName(id)` / `toStateId(display)` using bridge data (hardcode arrays to avoid JSON parse at runtime). Match manifest exactly for ids.
4. **Front flip affordance** (shared): In `FaceplateView`, add hitrect for brand/serial (or per Design flip-trio) + `std::function<void()> onFlipToRear;`. Wire in editor. (May be brand plate click per task prose.)
5. **New rear components** (CC primary, Grok support): Implement `RearPanelView` (bg load from new BinaryData, place() from anchors, inline or child rockers/wells/medallion). Implement `EngageRocker`, `CabSelectWell`/`NeuralSelectWell` (paint screen + opts per HTML CSS; handle clicks, chevrons for cycle, direct list select; map to ids; callbacks). Use Hultog for names (load typeface), Orbitron/amber for LEDs/titles. Match front inline-draw style for small footprint.
6. **Editor integration + flip** (CC/Grok): Add rear view member (or conditional). `createEditor` / ctor wires both views' callbacks to processor setters + refreshes. Timer: push current cab/neural/engage to active view's setters. Mouse on front brand → flip (hide front, show rear or swap paint). Rear medallion → flip back. Add rotation affordance later (450ms). Keep 960x600. Sync modified dot still works (front only).
7. **Wiring + sync** (CC): Rear rocker toggle → processor setter → (DSP) + repaint. Well select → setIr/setModel + repaint current readout. On preset load / slot / state recall: editor pulls from processor getters and pushes to both views. Test overrides survive A/B, dirty dot lights on rear change, revert works, no audio thread alloc.
8. **Polish + QA**: Dark/amber/rose-gold match (use tokens from `gui.md` + bypass-dimstate). Hit-test rects exact (no paint dependency). @2x crisp. Real-time: all rear changes msg-thread only. Add to `isDirty` already done. Update docs (`gui.md`, CLAUDE.md). Run full test: build, AudioPluginHost load, flip, engage IR/model, confirm meter/processing changes, no clicks, preset modified dot correct.
9. **Handoff closure**: Update `DESIGN_HANDOFF_STATUS.md` etc. if needed. Ensure BinaryData for any last-minute rear art.

**Risks / notes**: Flip animation may require `juce::ComponentAnimator` or custom transform (post-MVP?). Rear may share some footer code. If unified view (one Component with `bool rearMode`), paint branches on bg + children visibility. Follow "Never write actual DSP math into the stub files" — already done. All changes must clang-format + zero warnings.

**Evidence locations (absolute)**:
- Spec/anchors: `/Users/kalimeeks/Documents/GitHub/wk-grok-assets/docs/flip-spec.html:222-286` (CABINET/NEURAL modules), `rear-panel-anchors.json:10-33`.
- GUI current: `Source/gui/FaceplateView.{h,cpp}` (entire; esp. 109-206 resized/place, 258 mouseDown, 385 drawPreset), `Source/PluginEditor.{h,cpp}`.
- State/processor: `Source/PluginProcessor.{h,cpp}:104-109` (members), 171 (apply), 208 (isDirty), `Source/WonKnobberState.{h,cpp}:18-21` (fields), 20-30 (isKnown*).
- Assets/CMake: `CMakeLists.txt:58-86`, `Resources/rear-panel-background-*.png` (present but unembedded), `Resources/Fonts/` (Orbitron only).
- IDs/bridge: `docs/cab-neural-id-bridge.json:5-19` (state_id), `docs/WON_cab_neural_asset_manifest_2026-06-02.json:20-87` (ir_assets/neural_slots .id), `Source/dsp/{Convolution,NeuralModel}.h:23,25`.
- Reflection: `docs/modified-from-preset-indicator.md:16-20`, `Source/gui/FaceplateView.cpp:410` (ember for cab/neural).
- Other rear: `docs/rear-panel-bg.html`, `docs/gui.md`, `docs/preset-override-precedence.md`, `docs/DESIGN_HANDOFF_STATUS.md:11` (anchors landed), `prep/SUBAGENTS_FANOUT.md:26-27` (this task).

This prep is exhaustive for handoff. CC can scaffold directly from anchors + this map. Grok stands ready for BinaryData/processor/ID pieces.

---

**End of report.** (All tasks from user prompt completed via tools: list_dir, read_file with offsets, grep with patterns/paths/globs, todo tracking.)