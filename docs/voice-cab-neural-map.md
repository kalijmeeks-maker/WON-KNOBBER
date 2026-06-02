# WON KNOBBER — Authoritative 8-voice → Cab IR + Neural map (2026-06-02)

**From Claude Design (UI/UX lead). This supersedes Grok's proposed table.**
This is the canonical per-voice signal-chain identity. It drives:
- **Grok** — which of the 6 IRs each voice references (sourcing priority).
- **Claude Code** — the `cabIr` / `neuralModel` / `cabEngage` / `neuralEngage` values
  baked into each factory preset XML.

Chain order is fixed: **saturation → convolution (cab) → neural**. Drive/mix are the
existing shipped §1 values (gem column for reference). Cab + neural fold in per row.

## Canonical attribute keys (LOCKED — confirmed to CC 2026-06-02)
All three sides (CC `WonKnobberState`, Grok manifest, preset XMLs, rear-panel-anchors.json
bindings) use exactly these keys:

| Key | Type | Values |
|-----|------|--------|
| `cabIr`        | string | `FLAT` / `STUDIO RIBBON` / `VINTAGE 4x12` / `CONSOLE BOX` / `OLD RADIO` / `IRON CORE` |
| `neuralModel`  | string | `NONE` / `TAPE-1971` / `VALVE-CLASS A` / `TRANSISTOR-FET` / `IRON-TRANSFORMER` |
| `cabEngage`    | bool   | true / false |
| `neuralEngage` | bool   | true / false |

Note: `neuralModel` (not `neural`) is the canonical key — Grok's manifest schema_v2 example
must change `neural` → `neuralModel` to match.

| # | Voice | Gem | Drive | Mix | Cab IR | Neural | Cab eng. | Neural eng. |
|---|-------|-----|-------|-----|--------|--------|----------|-------------|
| 1 | TAPE HEAD    | diamond / 0.42 | 0.42 | 1.00 | STUDIO RIBBON | TAPE-1971        | on  | on  |
| 2 | CONSOLE GLUE | onyx / 0.30    | 0.30 | 1.00 | FLAT          | NONE             | off | off |
| 3 | FURNACE      | ruby / 0.86    | 0.86 | 1.00 | VINTAGE 4x12  | VALVE-CLASS A    | on  | on  |
| 4 | VELVET       | amethyst       | 0.38 | 0.90 | STUDIO RIBBON | TAPE-1971        | on  | on  |
| 5 | SUNDAY DRIVE | citrine        | 0.55 | 0.85 | OLD RADIO     | TRANSISTOR-FET   | on  | on  |
| 6 | TUBE WARM    | citrine        | 0.48 | 1.00 | CONSOLE BOX   | VALVE-CLASS A    | on  | on  |
| 7 | DIODE BITE   | emerald        | 0.70 | 0.95 | VINTAGE 4x12  | TRANSISTOR-FET   | on  | on  |
| 8 | TRANSFORMER  | sapphire       | 0.60 | 1.00 | IRON CORE     | IRON-TRANSFORMER | on  | on  |

## Notes / rationale
- **CONSOLE GLUE is deliberately bare** — FLAT cab + NONE neural. It's the "just the
  saturator" reference voice; both stages disengaged so A/B against it shows what cab +
  neural add. This is the one voice where the rear engage-rockers read OFF by default.
- **STUDIO RIBBON pairs with the dark/smooth voices** (TAPE HEAD, VELVET) — flattering
  top rolloff matches their character.
- **VINTAGE 4x12 carries the aggressive voices** (FURNACE, DIODE BITE) — mid bark + steep
  top.
- **IRON CORE on TRANSFORMER** — transformer-iron bloom, not a speaker box; matches the
  sapphire/iron identity.
- Drive/mix are placeholders pending the **WON QA A/B-by-ear pass** — expect small nudges.
  Cab/neural assignments above are **locked** regardless of the QA drive/mix tuning; QA may
  also A/B the cab/neural pairings, in which case send me the consolidated diffs and I'll
  reissue this table as the final.

## Open dependency
- Neural model names match the rear NEURAL select list. If Grok's RTNeural training yields
  a different model taxonomy, reconcile names back to this list (NONE / TAPE-1971 /
  VALVE-CLASS A / TRANSISTOR-FET / IRON-TRANSFORMER) so presets, rear UI, and assets agree.
