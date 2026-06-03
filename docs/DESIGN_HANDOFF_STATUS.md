# Design handoff — filesystem status (2026-06-02 pm)

| File | In repo? | Notes |
|------|----------|-------|
| `voice-cab-neural-map.md` | Yes (#43) | Authoritative voice map |
| `SYNC_TO_CC_cab_neural_direction.md` | Yes | §1–§4 rear-fold direction |
| `rear-panel-anchors.json` | Yes (**#51** on `main`) | Re-measured coords — use this for PR4 |
| `modified-from-preset-indicator.md` | Yes | 4-field dirty rule (PR #50) |
| `preset-override-precedence.md` | Yes | Stamp-six / persist-until-load |
| `rear-panel-bg.html` | Yes | Source mock; baked PNGs in `Resources/` |
| `rear-panel-background-*.png` | **PR #52** | Rose/amber re-bake from Design zip `(2)` 2026-06-02 |
| `bypass-dimstate-tokens.json` | Yes (#46) | Design-canonical |
| `cab-neural-id-bridge.json` | Yes (#43) | UI display ↔ `state_id` |

## Still owed from Design (PR4 blockers)

- **Flip spec / flip-trio** — not present in the Jun 2 design-system zip; corner-screw → rear rotation wiring waits on this.
- `CURRENT_VERSIONS.md` inside the zip is stale (claims anchors uncommitted; #51 landed).

**Recurrence fix:** Design workspace → Kali path drop → Grok PR (`repo-handoff/` only) → CC merges → PR4 paint.