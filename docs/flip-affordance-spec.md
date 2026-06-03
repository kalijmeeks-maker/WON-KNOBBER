# Flip Affordance / Transition Spec (extracted from flip-spec.html + rear-panel-anchors.json)

**Source:** docs/flip-spec.html (the rear service panel prototype) and docs/rear-panel-anchors.json (coords + notes).  
**Date extracted:** 2026-06-03 (post subagent prep).  
**Purpose:** For building the flip-to-rear / flip-to-front transition in the UI (pending Design's final screw-vs-⟳ visual confirm for the front affordance).

## Rear View (the "SERVICE" side)
- The rear is a 960×600 chassis (rose-gold polished radial + texture bg, corner screws matching front .pro-stage .mk-screw style: 18px, radial-gradient from #ffe3d8 to #4a2418 with slot highlight).
- Header: brand SVG (Hultog/CF Engraved "WON" gold-gradient + "KNOBBER" silver), "REAR · SERVICE PANEL" tag, spec-inset (Orbitron "JUCE 8.0.13 · v1.0.0" + "FIRMWARE · BUILD"), head-stamp SERIAL "WK—0001" amber-engraved.
- Body 3-zone grid:
  - LEFT: CABINET module (recessed). Engage rocker (64×30 cap slider, .off state cap left/cool, .on cap right/amber) + amber jewel LED. Label "IMPULSE RESPONSE · CONVOLUTION". Screen with current IR (Hultog ~17px amber + ◂ ▸ chevrons, e.g. "VINTAGE 4×12"). Below: .opts list (engraved): FLAT, STUDIO RIBBON, VINTAGE 4×12 (on=bright + glow), CONSOLE BOX, OLD RADIO, IRON CORE.
  - CENTER (hero): "FLIP TO FRONT" (eng md). .hero-well with rose-gold .medallion (128×128, conic+radial, engraved "↻" + "FLIP", cursor:pointer). .hero-readout "SAT → CAB → NEURAL". .hero-scale labels FRONT / SERVICE / REAR.
  - RIGHT: NEURAL module (symmetric to left). Engage rocker+LED. "MODEL · RTNEURAL". Screen e.g. "TAPE — 1971". .opts: NONE, TAPE — 1971 (on), VALVE — CLASS A, TRANSISTOR — FET, IRON — TRANSFORMER.
- Footer: oversampling segmented (OFF / ×2(on amber) / ×4 / ×8), I/O Trim strip (value + chevrons), foot-btns (ABOUT / MANUAL).
- The flip control on rear is the center **medallion** (rose-gold disc with ↻ FLIP icon/text). Click/activate rotates chassis 180deg back to front (~450ms cubic-bezier(.2,.8,.2,1) per note in anchors).
- Anchors for medallion: "flip_medallion": [416, 228, 128, 128]; "_flip_medallion_note": "Live hit-target. Click rotates chassis 180deg back to front (~450ms cubic-bezier(.2,.8,.2,1)). Static rose-gold disc is baked; only the click + rotation are wired."

## Front View (the "FRONT" side) — Affordance (pending confirm)
- The front is the existing 960×600 PRO chassis (faceplate_pro_960x600.png baked).
- The flip-to-rear affordance is **not yet implemented** in current FaceplateView (per subagent gap analysis: no hitrect on brand/serial or screws for flip; mouseDown only handles bypass, presets, transport, about, modified dot).
- From the rear prototype + anchors context + broadcast: the affordance on front is expected to be on the **brand/serial area** (or a corner screw, "screw-vs-⟳ confirm" pending from Design). Likely the brand SVG or the head-stamp "SERIAL WK—0001" or one of the 4 corner screws acts as hit-target to flip to the rear service panel.
- On activation: hide/show or animate the view to the rear (chassis "rotates" to show the service side with the two modules + medallion for flip back).
- The transition should be the 180deg rotation (as specified for the medallion on rear).

## Anchors (for layout, multiply ×2 for @2x)
- "chassis": [0, 0, 960, 600]
- "flip_hero_well": [386, 198, 188, 188]
- "flip_medallion": [416, 228, 128, 128]
- Other relevant for modules: cabinet_module [41,119,299,380], neural_module [620,119,299,380], etc.
- _bg_asset: rear-panel-background-960x600.png (@2x: rear-panel-background-2x-1920x1200.png)
- _note: "Rear panel reworked to match the FRONT faceplate language (rose-gold mk-chassis-rg frame, warm near-black mk2-panel field, pro-module recesses, mk2-rule amber hairlines, jewel LEDs). ... CC paints the live layers below; static labels/recesses are already baked."

## IDs for the Wells (must match manifest/state form for storage, display for painting)
- Cab IR opts (state): FLAT, STUDIO_RIBBON, VINTAGE_4X12, CONSOLE_BOX, OLD_RADIO, IRON_CORE
- Neural opts (state): NONE, TAPE, VALVE, TRANSISTOR, IRON
- Use display names from the HTML/anchors for .cur and .opts (e.g. "VINTAGE 4×12", "TAPE — 1971") when painting; map via cab-neural-id-bridge.json when storing/reading from processor (currentCabIr etc use state ids).

## Pending from Design (per broadcast + subagent)
- Screw vs ⟳ confirm for the **front** flip affordance (is it a corner screw hit, or the brand/serial plate, or a dedicated ⟳ icon?).
- Final rear anchors PNG (the baked comp with live layers painted).
- Any updates to flip-trio (corner-screw affordance spec).
- The subagent PREP_REAR_UI_READINESS.md has the full mapping, component suggestions (e.g. RearPanelView or inline draws matching front bypass style, EngageRocker, CabSelectWell/NeuralSelectWell), and order: Grok did embed + API; CC scaffolds the views + wiring to the (now canonical) processor setters; shared editor flip + timer sync.

## Implementation Notes for Build
- Front: add hitrect (e.g. on brand or serial stamp in FaceplateView) + onFlipToRear callback. Animate or swap to rear view (or conditional paint mode).
- Rear: use the anchors coords with the same place() scaling as FaceplateView (kRefW=960, kRefH=600). Load rear bg from BinaryData (now embedded). Draw/paint live rockers, screens, wells, medallion per the CSS in flip-spec.html (gradients, shadows, Hultog for names, Orbitron for LEDs, amber/rose-gold tokens).
- Flip transition: on medallion (rear) or front affordance: 180deg rotation affordance (~450ms cubic-bezier(.2,.8,.2,1)).
- State: use the public getters/setters on processor for cabEngage/currentCabIr etc (and neural). The 4 fields already drive isDirty (the ember dot), revert, A/B, loadedVoice.
- No front face changes (per broadcast).
- Fonts: Hultog_Engraved + CF_Engraved for engraved text in wells (still owed from Design; front currently fakes engraved with system bold).

**Extracted for readiness (pending the screw-vs-⟳ confirm).** See also PREP_REAR_UI_READINESS.md for more (gaps evidence, full component list, suggested PR order).

This spec is now in docs/ for the team.
