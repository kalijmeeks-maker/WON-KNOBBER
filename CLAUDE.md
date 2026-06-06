# Project: WON-KNOBBER — JUCE Audio Plugin

## Targets (HARD CONSTRAINT)
- **AUv3 + VST3 ONLY.** macOS only. (NOT AUv2 / `.component` — per owner directive 2026-06-06.)
- Do NOT add AAX, iLok, Windows, or Linux targets. No standalone unless asked.
- C++20, JUCE 8.x, CMake (Pamplejuce-style), Catch2 for tests.

## Architecture (carry-forward spec — do not deviate without asking)
- **Faceplate + seated overlays:** Photoreal PRO chassis as background image.
  Controls are live overlays drawn into the plate's recesses/wells —
  "seated, not painted-on." Never paint controls as floating widgets.
- **Reference-coordinate layout:** ONE `kRefW x kRefH` reference. Every control
  placed via `place(x, y, w, h)` that scales from a single plate-truth JSON.
  All anchors come from that JSON — no hardcoded magic numbers in resized().
- **DSP chain (RT-safe processBlock):**
  Airwindows saturation (oversampled) → 6-IR convolution → RTNeural →
  equal-power dry/wet.
- **Oversampling:** 32x offline / 16x realtime as the MAX. Use
  juce::dsp::Oversampling; switch factor on isNonRealtime().
- **Controls:** one meaningful macro (drive) + gem filmstrip knob
  (120 frames, min-first, inversion-free LookAndFeel, 7 stone variants).
  Identity = amber/dark.
- **State:** raw addParameter + custom magic-header XML blob DECOUPLED from
  the param list. Adding params must NOT break saved sessions.
- **Host behavior:** bypass param + PDC, automation begin/endChangeGesture,
  modal scrim + Esc to dismiss. Cab/neural live on a REAR panel; front stays clean.

## Verification (give yourself a check you can run — ALWAYS run these)
- `ctest` (Catch2 unit tests) must pass.
- `pluginval --strictness-level 10` must pass on both AU and VST3.
- `auval -v aufx <subtype> <manu>` must pass for the AU.
- Headless render harness: createComponentSnapshot → PNG on the REAL editor
  (not a detached view), diffed against golden PNG. Must match within tolerance.
- processBlock must be RT-safe: NO allocation, locks, logging, or file I/O.

## Workflow rules
- Explore → Plan → Code → Commit. Use plan mode; let me approve the plan first.
- Run the verification loop after every meaningful change. Don't trust, verify.
- When context gets noisy, /clear and re-read this file + critical-patterns.md.
- Required reading before touching DSP or GUI: ./docs/juce-critical-patterns.md
