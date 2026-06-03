# Modified-from-preset indicator (Design-canonical, 2026-06-02)

**Not gated on rear-fold lock.** Wire with preset bake + preset strip UI.

## Visual

- **Dot:** 6px circle after `.mk-preset-name`, vertically centred, `margin-left: 2px`
- **Fill:** `radial-gradient(circle at 40% 35%, #ffd28a 0%, #ff8800 45%, #a35d00 100%)`
- **Glow:** `box-shadow: 0 0 7px 1px rgba(255,150,30,.80)`
- **Motion:** optional 2.4s opacity breathe; disable under `prefers-reduced-motion`

Reference comp: `modified-dot-spec.html` (Design handoff when committed to repo).

## When to show (Design-locked 2026-06-02 — narrow rule)

`isDirty` when **any of the four identity fields** differ from the loaded factory voice snapshot:

- `cabIr`, `neuralModel`, `cabEngage`, `neuralEngage`

**Not** `drive` / `mix` — riding the main knobs is expected; dot surfaces hidden rear cab/neural overrides on the front face.

Hidden when clean.

## Anchor (locked)

- Immediately **right of** `.mk-preset-name`, vertically centred (`align-self: center`), `margin-left: 2px`
- **Hit-target:** 20×20 px transparent wrap centred on 6 px dot, `cursor: pointer`

## Clear / revert

- Clears on preset load (preset still stamps all six fields — see `preset-override-precedence.md`)
- Click dot → revert **four identity fields** only; leave `drive` / `mix` as the user set them
- **Tooltip:** `MODIFIED FROM {VOICE} · CLICK TO REVERT`

## Precedence (see `preset-override-precedence.md`)

Loading a preset stamps all six; manual edits persist until next load; dot is divergence signal only — never mutates stored factory preset XML.