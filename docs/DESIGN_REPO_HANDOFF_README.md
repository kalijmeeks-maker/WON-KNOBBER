# WON KNOBBER — Design → Repo handoff (2026-06-02)

Drop these into the WON-KNOBBER repo so Claude Code can read them (PRs 2–4 + preset-XML
population are blocked until they're committed). Suggested destinations:

```
docs/
  voice-cab-neural-map.md              → authoritative 8-voice → cabIr/neuralModel map
  SYNC_TO_CC_cab_neural_direction.md   → the full §1–§4 direction
  rear-panel-anchors.json              → live-layer anchors for the rear UI (960×600)
  rear-panel-bg.html                   → rear panel source (re-bake @2x from this if needed)

Resources/
  rear-panel-background-960x600.png    → baked rear plate
  rear-panel-background-2x-1920x1200.png → @2x (viewport-upscaled; re-bake from the HTML
                                           via the front-faceplate pipeline for crispness)
```

## Canonical attribute keys (LOCKED)
`cabIr` (string) · `neuralModel` (string) · `cabEngage` (bool) · `neuralEngage` (bool).
Matches CC's `WonKnobberState`. Grok manifest must rename `neural` → `neuralModel`.

## Notes
- Rear panel was reworked to match the FRONT faceplate language (rose-gold frame, warm
  near-black field, recessed modules, amber rules, jewel LEDs). Coordinates in the anchors
  JSON are the NEW layout — supersede any earlier rear coords.
- Front-panel files are already in the repo; nothing there changed except the brand
  wordmark now uses Hultog Engraved (shared BrandMark component) — re-bake the front comp
  if you want the PNG to reflect it.
