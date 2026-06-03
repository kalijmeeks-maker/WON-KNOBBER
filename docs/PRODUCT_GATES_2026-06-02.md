# Product gates (2026-06-02) — do not skip

## Gate A — Rear-fold lock (Kali only)

**Status:** UNCONFIRMED (leaned, not locked).

If **locked (A):** cab/neural in 8 voices; rear override; front one-knob; entry via bottom-right corner screw flip (Design spec pending).

If **veto (B):** front-face cab/neural = full faceplate re-spec — **do not merge PR 4 / rear handoff as-is.**

## Gate B — Front ↔ rear bridge (Design, after A)

Corner-screw flip affordance + 180° transition spec/anchors. Blocks PR 4 implementation.

## Gate C — Preset override (Design shipped; CC implements)

See `preset-override-precedence.md` + `modified-from-preset-indicator.md`. **Not blocked on A.**

## Stack merge (Kali)

Nod to include Grok **#40** in flatten: #41 → #40 → #42 → one of #43/#44 → #45 → #46.