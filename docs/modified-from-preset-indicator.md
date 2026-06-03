# Modified-from-preset indicator (Design-canonical, 2026-06-02)

**Not gated on rear-fold lock.** Wire with preset bake + preset strip UI.

## Visual

- **Dot:** 6px circle after `.mk-preset-name`, vertically centred, `margin-left: 2px`
- **Fill:** `radial-gradient(circle at 40% 35%, #ffd28a 0%, #ff8800 45%, #a35d00 100%)`
- **Glow:** `box-shadow: 0 0 7px 1px rgba(255,150,30,.80)`
- **Motion:** optional 2.4s opacity breathe; disable under `prefers-reduced-motion`

Reference comp: `modified-dot-spec.html` (Design handoff when committed to repo).

## When to show

`isDirty` when **any** of these differ from the loaded factory voice snapshot:

- `drive`, `mix`, `cabIr`, `neuralModel`, `cabEngage`, `neuralEngage`

Hidden when clean.

## Clear / revert

- Clears on preset load (clean stamp of all six fields)
- Click dot → revert all six to loaded voice values
- **Tooltip:** `MODIFIED FROM {VOICE} · CLICK TO REVERT`

## Precedence (see `preset-override-precedence.md`)

Loading a preset stamps all six; manual edits persist until next load; dot is divergence signal only — never mutates stored factory preset XML.