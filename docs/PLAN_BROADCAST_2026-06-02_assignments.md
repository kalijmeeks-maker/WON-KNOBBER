# WON-KNOBBER — Long-Run Work Assignments (2026-06-02)

From Claude Code, relayed via Kali. Deep-runway assignments so each lane runs a long
stretch without round-tripping.

## Operating model

**Claude Design leads all design/aesthetics** — Kali + Design own the visual loop
(mockups, markups, color/opacity tokens, pixel-checks). **Claude Code + Grok own the heavy
non-design work** and bring anything visual *back* to Design rather than deciding it.

## Claude Code — engineering queue (no design calls required)

1. **Flatten the stack** — merge #41 → #40 → #42 → #44 to `main` once CI is green
   (pending Kali's nod on Grok's #40), so the rest builds off clean main.
2. **PR 3 — RTNeural stage:** FetchContent RTNeural @ pinned SHA, embed the 5 model JSONs,
   wire `NeuralModel` (load-by-id, atomic engage gate, RT-safe, zero added latency for
   feedforward), chained after convolution. Defaults off. Smoke self-tests.
3. **Preset-value bake:** write `cabIr` / `neuralModel` / `cabEngage` / `neuralEngage`
   (machine ids mapped from the voice map) into the 8 factory XMLs; cab/neural assignments
   locked, drive/mix left for QA. Update factory-embed tests.
4. **Bypass dim-state mechanics:** `setBypassed` plumbing across LEDs / scopes / meters /
   gem / dB / arc + scope-freeze + 180 ms transition. §3 numbers wired as **placeholders**;
   exact colors/opacities are **Design's to finalize** (exposed as easily-swapped tokens).
5. **PR 4 — rear UI:** place + wire every live layer from `rear-panel-anchors.json`
   (2 rockers, 2 LEDs, IR/model wells + option lists, flip medallion, oversampling segment,
   I/O-trim, About/Manual). Layout from anchors; look/feel = Design pixel-check after.
6. **Plumbing:** "View full licences" → real `THIRD_PARTY_LICENSES.md` scroll modal; rear
   oversampling segment → an actual OS param wired to the saturation oversampler.

CC ships these as stacked PRs and reports links without pinging per-PR.

## Grok + agents — asset/research queue (parallel, long-horizon, no visual work)

1. **Replace the 6 synthetic IR placeholders** with real/high-fidelity captures hitting each
   §2 character curve — **same filenames**, license-clean, within tap budget.
2. **Train the 4 RTNeural models** (TAPE / VALVE / TRANSISTOR / IRON) to character, export
   RTNeural JSON under the same filenames; NONE = passthrough; validate RT-safe + 0 latency.
   *The big autonomous one — days of runway.*
3. **Per-IR + per-model license notices** → into `THIRD_PARTY_LICENSES.md` (feeds CC's
   licences scroll).
4. Keep manifest + RTNeural pin current, asset-branch CI green, embed under the ~350 KB
   budget. Don't touch the faceplate — that's Design.

## Kali — your loop (with Claude Design)

1. **Aesthetic markups:** you tweak mockups/markups → Design canonicalizes into tokens
   (dim-state opacities/colors, rear material, About fonts/layout, @2x crispness) → CC
   applies. CC makes **no** aesthetic calls.
2. **Pixel-check loop:** once CC lands bypass dim-state + rear UI, grab Ableton screenshots
   (bypassed face, About, rear) → Design pixel-checks vs §3 → markups → CC adjusts.
3. **Calls only Kali makes:** nod to merge Grok's #40; front-vs-rear product confirm
   (leaning rear); after WON QA's by-ear pass, the final drive/mix numbers.
4. Relay glue between Code / Grok / Design.

## Claude Design — leadership

Owns every visual decision — dim-state tokens, rear material/feel, About layout/fonts,
final sign-off. Code/Grok build to spec and bring renders back for pixel-check. Re-bake the
rear @2x crisp when able; reissue any tokens and CC applies them.

## Live PR state (2026-06-02)

| PR | What | State |
|----|------|-------|
| #38 | bypass + dim-state + About | About now verbatim per §3; dim-state mechanics next |
| #40 | Grok cab/neural assets | tracking |
| #41 | WonKnobberState cab/neural fields | open |
| #42 | embed 6 IRs + zero-latency Convolution | open (stacked on #41+#40) |
| #44 | rear-panel design handoff (docs + rear PNG) | open |

**Merge order:** #41 + #40 → #42 → #44 → then PR 3 / PR 4.
