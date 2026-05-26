# WON-KNOBBER

A photorealistic **single-knob** audio processing plugin (VST3 / AU / AUv3) with a vintage, rack-mounted analog aesthetic — inspired by the design language of Acustica Audio's Acqua / NEBULA engine, with the modern polish of UAD and FabFilter.

One knob. One job. Rendered like real hardware.

---

## The idea

Most plugins drown the user in controls. WON-KNOBBER does the opposite: a single large rotary knob dominates a brushed-metal faceplate, flanked by a glowing analog VU meter and an LED bypass indicator. The knob drives one musically meaningful parameter (drive / saturation, threshold, EQ tilt, or reverb mix), backed by convolution or neural modeling for authentic analog character — the NEBULA approach.

The visual goal is photorealism: anodized aluminum, engraved typography, corner Phillips screws, micro-grain, subtle wear, drop-shadowed knob with depth shading and reflection. Dark-mode only.

---

## Aesthetic layers

- **Industrial hardware** — brushed/anodized metal, corner screws, rack-unit proportions (Acustica / UAD).
- **Vintage analog** — warm amber-to-red VU metering, engraved metal labels, LED-style digital readout.
- **Premium materials** — optional glass, marble, carbon-fiber, and crystal panel variants (FabFilter-modern luxury).

---

## Design tokens

- **Palette:** deep charcoal / gunmetal / dark olive base; warm amber, orange, or teal accents; brushed-metal engraved text with subtle glow on active params.
- **Knob:** large, centered, 0–360° (endless-style preferred), smooth animated rotation, engraved grip ridges, highlights + shadows for 3D realism.
- **Layout:** compact single-row faceplate, ~400×150 to 500×200 px; knob centered; meters/indicators flanking; engraved logo top or bottom.
- **Typography:** engraved sans for labels (CF Engraved / Hultog Engraved), Orbitron + Audiowide for LED/branding, Fontaudio icon set for transport/UI glyphs.
- **Interaction:** drag/click with value animation, hover tooltips, real-time meter response, preset browser, MIDI learn.

---

## Tech stack

- **Framework:** JUCE (C++20), cross-platform.
- **Rendering:** JUCE Graphics for the MVP; Skia GPU-accelerated rendering planned post-MVP. SVG-based scalable assets.
- **DSP:** convolution / neural modeling for analog emulation (NEBULA-style).
- **Formats:** VST3 (all platforms) + AU / AUv3 (macOS).

---

## Project layout

```
WON-KNOBBER/
├── Source/            # C++ source (PluginProcessor, PluginEditor, dsp/, gui/, util/)
├── Resources/         # asset tree (Faceplates, Knobs, Meters, Fonts, IRs, Models) — .gitkeep placeholders
├── docs/              # architecture, dsp, gui, conventions, glossary
├── .github/           # CI workflow, issue/PR templates, CODEOWNERS, Dependabot
├── .claude/           # Claude Code settings + slash commands
├── CMakeLists.txt     # JUCE via FetchContent; juce_add_plugin (VST3 + AU + AUv3)
├── CONTRIBUTING.md
├── SECURITY.md
├── LICENSE
└── README.md
```

---

## Build

Requires **CMake 3.22+** and a **C++20** toolchain. JUCE is pulled automatically via `FetchContent` — no submodule or system install needed.

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Built plugins land under `build/`. Load the VST3/AU in your DAW or JUCE's `AudioPluginHost`.

---

## Design assets

Curated reference + raw assets (brushed-metal faceplate, photoreal knob, VU meter reference, material panels, engraved/LED fonts, Fontaudio SVG icon set) feed the design-system generator. Asset folders ship empty (`.gitkeep`); no binaries are committed.

**Fonts:**

- CF Engraved & Hultog Engraved (DaFont)
- Orbitron & Audiowide (Google Fonts, OFL)
- Fontaudio (MIT)

---

## Roadmap

- [ ] Core JUCE plugin skeleton (VST3 + AU + AUv3 targets)
- [ ] Photoreal faceplate + knob rendering (filmstrip MVP → Skia)
- [ ] Single-parameter DSP module (drive/saturation MVP)
- [ ] Analog VU meter with ballistics
- [ ] LED bypass indicator + click animations
- [ ] Convolution engine (IR loading)
- [ ] Neural model inference path
- [ ] Preset browser + MIDI learn
- [ ] Alt material variants (glass, marble, carbon fiber, crystal)

---

## Status

Public, source-available, solo-maintained experimental project. Branch protection is active on `main` (PRs required, no force-push/deletion).

---

## License

Licensed under the **PolyForm Noncommercial License 1.0.0** — free to use, modify, and share for **noncommercial** purposes; commercial use requires a separate license. See [`LICENSE`](LICENSE).

Third-party dependencies and assets retain their own licenses (OFL fonts, Fontaudio MIT, etc.).

> **JUCE note:** JUCE is dual-licensed. The free path is GPLv3; shipping a **closed-source commercial** build requires a JUCE Indie/Commercial license. Sort this out before any paid release.
