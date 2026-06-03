# Preset vs manual override (Design-canonical, 2026-06-02)

Applies whether cab/neural are exposed on rear or front.

## Rules

1. **Load preset** → stamps `drive`, `mix`, `variant` (gem), `cabIr`, `neuralModel`, `cabEngage`, `neuralEngage` from factory voice.
2. **User edits** (rear wells, rockers, knobs) → persist in live state until the **next** preset load.
3. **Factory preset files** on disk are **not** rewritten by user tweaks.
4. **Modified dot** (`modified-from-preset-indicator.md`) → UI signal when **cabIr / neuralModel / cabEngage / neuralEngage** diverge from last loaded voice (not drive/mix).

## CC implementation notes

- Compare live state to `lastLoadedVoiceSnapshot` (or equivalent) for `isDirty`.
- A/B slots save/load include cab/neural fields when schema v2 is active.
- Preset bake: use `cab-neural-id-bridge.json` machine ids for XML; drive/mix per WON QA when available.