# WON-KNOBBER — Plan Broadcast (2026-06-02)

From Claude Code, relayed via Kali. Marching orders for the three-agent lane split
(Claude Design · Grok · Claude Code) toward the v1 functional finish.

## Where things stand

- `main` is feature-complete through **#37** (GUI + full preset stack: 8 voices, A/B,
  transport, gem picker, meters, scopes).
- **PR #38 is OPEN** (`feat/bypass-dimstate-about`): functional **bypass** (atomic,
  honored in `processBlock` as a true dry passthrough) + full-faceplate **dim-state
  veil** + footer rocker, and the **About / MIT panel** — which satisfies the
  `THIRD_PARTY_LICENSES.md` ship requirement to surface the Airwindows notice in-app.
  Builds clean JUCE 8 `-Werror`, **auval PASS**, self-tests green. Not yet visually
  verified in a DAW.
- **License credit corrected** on #38: `© 2026 Kali Meeks` / "WON-KNOBBER licensed
  under PolyForm Noncommercial 1.0.0", with the Airwindows `© 2018 Chris Johnson — MIT`
  credit and a pointer to `THIRD_PARTY_LICENSES.md`. Vendor field
  `COMPANY_NAME="kalijmeeks-maker"` left as-is — that's the brand, distinct from the
  copyright holder.
- The **only v1 functional gap left** is cab (`Convolution`) + neural (`NeuralModel`),
  still stubs. Everything below serves closing that.

## CLAUDE DESIGN — now, in priority order

1. **Deliver your files where CC can read them.**
   `SYNC_TO_CC_cab_neural_direction.md` and the visual board are **not on CC's disk**
   (only the older sync docs are). Commit them to `docs/` or the design repo so CC can
   (a) match your exact About-card license wording verbatim before #38 merges, and
   (b) work from the real spec rather than the relayed gist.
2. **Rear-panel anchors + background PNG** — the real unblock for cab/neural UI.
   Produce the 960×600 rear-panel anchor JSON (CABINET engage-rocker + IR select-well,
   NEURAL engage-rocker + model select-well, flip-screw affordance, brand/serial plate)
   plus the brushed-steel rear background PNG. CC cannot place components without it.
3. **Per-voice cab + neural map.** Lock which of the 6 IRs and which neural model each
   of the 8 factory voices carries. Feeds both Grok's sourcing and CC's preset schema.

## GROK — now (green-lit, runs fully in parallel)

Deliver the **IR shortlist + RTNeural-vs-ONNX spike** as a doc + asset manifest CC can
integrate against. Build against the **preset model** (cab/neural fold into the 8
voices + rear-panel manual override) — **not** a front-face IR picker.

- **6 IRs** — FLAT, STUDIO RIBBON, VINTAGE 4×12, CONSOLE BOX, OLD RADIO, IRON CORE.
  Each: license **redistributable + PolyForm-NC-compatible** (GPL / CC-BY-SA /
  no-redistribute = disqualifier); WAV; mono vs true-stereo; length in taps;
  BinaryData-embeddable size; **underscore filenames** (JUCE strips dashes from symbols).
- **Neural spike** — NONE / TAPE / VALVE / TRANSISTOR / IRON. Answer: build footprint
  under our CMake `FetchContent` / C++20 / no-system-install rule (RTNeural header-only
  vs ONNX prebuilt lib — must stay green on macOS **and** Windows CI); RT-safe load
  story (scratch pre-allocated in `prepare()`, **zero alloc/lock/IO in `process()`**,
  model loaded off the message thread); **latency in samples**; model format + license;
  a concrete recommendation with per-option integration cost.
- **Hard constraint (both):** RT-safe **swap-on-preset-load** — IR and model change with
  the preset, no click / dropout.

## CLAUDE CODE — holding, by design

Integration waits on Grok's asset shapes (wiring against guessed formats = rework).
PR #38 is up for review meanwhile. The moment the spike lands, CC takes the
`cabIR` / `neuralModel` preset-schema fields + `Convolution` engine wiring; the moment
Design's rear anchors land, CC scaffolds the rear-panel components.

## NEEDS KALI

**Confirm the rear-panel approach** (cab/neural fold into voices + flip-to-rear manual
override, front face untouched). Design's recommendation is sound but it gates their
anchor/PNG bake. If cab/neural should go on the **front** face instead, flag it now —
Design says that's a full faceplate re-spec, not a bolt-on.
