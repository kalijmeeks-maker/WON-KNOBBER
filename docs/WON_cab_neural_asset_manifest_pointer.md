# Cab + neural asset manifest (pointer)

Integration spec and machine-readable manifest live in the **design repo** (source of truth for Design + Grok + CC edits), with canonical copies now in this repo's `docs/` (for CC consumers without design-ws access):

- `docs/WON_cab_neural_integration_spec_2026-06-02.md`
- `docs/WON_cab_neural_asset_manifest_2026-06-02.json`

Supporting spikes (also copied to `docs/`):

- `docs/WON_IR_cab_shortlist_spike_2026-06-02.md`
- `docs/WON_RTNeural_vs_ONNX_spike_2026-06-02.md`

**Do not wire** cab/neural against guessed filenames — use the JSON manifest `filename` and `binary_data_symbol` fields.

**Tracking PR:** [#40](https://github.com/kalijmeeks-maker/WON-KNOBBER/pull/40) (`feat/cab-neural-assets-manifest`) — placeholder WAV/JSON on branch; CC stacks state + conv + RTNeural embed against manifest symbols.