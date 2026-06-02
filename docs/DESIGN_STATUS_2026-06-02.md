# Claude Code → Claude Design — Status Update (2026-06-02)

## Preset stack is fully shipped to `main`

All three PRs merged in dependency order:
- **#34** preset I/O foundation (Grok) → **#35** preset strip + transport UI → **#36** the
  **8 factory voices** from Design's §1 drive/mix/gem map.
- Gem-per-voice identity is live: loading a preset swaps the hero stone.

| # | Voice | drive | mix | gem |
|---|-------|-------|-----|-----|
| 0 | TAPE HEAD | 0.42 | 1.00 | diamond |
| 1 | CONSOLE GLUE | 0.30 | 0.85 | onyx |
| 2 | FURNACE | 0.86 | 1.00 | ruby |
| 3 | VELVET | 0.50 | 0.70 | amethyst |
| 4 | SUNDAY DRIVE | 0.38 | 0.90 | citrine |
| 5 | TUBE WARM | 0.60 | 1.00 | citrine |
| 6 | DIODE BITE | 0.72 | 0.80 | emerald |
| 7 | TRANSFORMER | 0.55 | 0.95 | sapphire |

## Validation done

- Builds clean JUCE 8 `-Werror` on macOS + Windows CI.
- **`auval` PASS** on the installed AU; all static-init self-tests green (preset API n=8,
  factory-embed round-trips for TAPE HEAD + FURNACE, A/B slot save/load, randomize, legacy
  roundtrip).
- **In-DAW 8-voice cycle capture in progress** (Ableton) to confirm gem + drive + mix
  visibly change per voice.

## Decision locked (resolves §6 P0.1)

Kali chose **v1 keeps ALL THREE DSP stages — saturation + cab (Convolution) + neural
(NeuralModel).** The stubs become real; nothing is cut. This is opposite the "cut if pure"
lean and reshapes the roadmap.

## Direction needed from Design (priority order)

1. **THE unblock — cab + neural controls:** visible controls on the 960×600 (departs from
   pure one-knob) vs preset-/menu-driven (protects one-knob identity)? If visible: layout +
   look for cab on/off + IR-select and neural on/off + model-select. Gates the whole
   cab+neural DSP lane.
2. **Cab voice direction** — which box/cab characters fit the aesthetic (feeds the IR set
   Grok will source).
3. **Bypass dim-state visuals** (offered) + **About panel** layout — §6 P1/P2, still pending.
4. **Preset parity:** mirror the final 8 drive/mix values into the web prototype after the
   WON QA ear-tuning pass, and run the matching sweep?

## Next preset slice

WON QA A/B-by-ear nudges to the starting values, as an XML-only follow-up PR (won't touch
wiring).
