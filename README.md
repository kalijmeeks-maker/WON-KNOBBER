# WON-KNOBBER

A photorealistic **single-knob** audio processing plugin (VST3 / AUv3) with a vintage, rack-mounted analog aesthetic — inspired by the design language of Acustica Audio's Acqua / NEBULA engine, with the modern polish of UAD and FabFilter.

One knob. One job. Rendered like real hardware.

---

## The idea

Most plugins drown the user in controls. WON-KNOBBER does the opposite: a single large rotary knob dominates a brushed-metal faceplate, flanked by a glowing analog VU meter and an LED bypass indicator. The knob drives one musically meaningful parameter (drive / saturation, threshold, EQ tilt, or reverb mix), backed by convolution or neural modeling for authentic analog character — the NEBULA approach.

The visual goal is photorealism: anodized aluminum, engraved typography, corner Phillips screws, micro-grain, subtle wear, drop-shadowed knob with depth shading and reflection. Dark-mode only.

---

## Aesthetic layers

- **Industrial hardware** — brushed/anodized metal, corner screws, rack-unit proportions (Acustica / UAD).
- - **Vintage analog** — warm amber to red VU metering, engraved metal labels, LED-style digital readout.
  - - **Premium materials** — optional glass, marble, carbon-fiber, and crystal panel variants (FabFilter-modern luxury).
   
    - ---

    ## Design tokens

    - **Palette:** deep charcoal / gunmetal / dark olive base; warm amber, orange, or teal accents; brushed-metal engraved text with subtle glow on active params.
    - - **Knob:** large, centered, 0–360° (endless-style preferred), smooth animated rotation, engraved grip ridges, highlights + shadows for 3D realism.
      - - **Layout:** compact single-row faceplate, ~400×150 to 500×200 px; knob centered; meters/indicators flanking; engraved logo top or bottom.
        - - **Typography:** engraved sans for labels (CF Engraved / Hultog Engraved), Orbitron + Audiowide for LED/branding, Fontaudio icon set for transport/UI glyphs.
          - - **Interaction:** drag/click with value animation, hover tooltips, real-time meter response, preset browser, MIDI learn.
           
            - ---

            ## Tech stack

            - **Framework:** JUCE (C++), cross-platform — Windows VST3, macOS AUv3.
            - - **Rendering:** Skia (or nanoVG) GPU-accelerated; SVG-based scalable assets.
              - - **DSP:** convolution / neural modeling for analog emulation (NEBULA-style).
               
                - ---

                ## Project layout

                ```
                WON-KNOBBER/
                ├── Source/              # C++ source (PluginProcessor, PluginEditor, DSP, GUI)
                ├── Assets/              # SVGs, textures, knob frames, VU graphics
                │   ├── Faceplates/
                │   ├── Knobs/
                │   ├── Meters/
                │   └── Fonts/
                ├── Resources/           # Impulse responses, neural model weights
                ├── JuceLibraryCode/     # JUCE-generated bindings (Projucer)
                ├── Builds/              # Per-platform build folders (ignored)
                └── README.md
                ```

                ---

                ## Build

                Requires JUCE 7+ and a C++17 toolchain.

                **macOS (AUv3 / VST3):**
                ```bash
                # Open the .jucer file in Projucer, then export to Xcode
                open WON-KNOBBER.jucer
                ```

                **Windows (VST3):**
                Open the exported Visual Studio 2022 solution from `Builds/VisualStudio2022/`.

                **CMake (optional):**
                ```bash
                cmake -B build -S .
                cmake --build build --config Release
                ```

                ---

                ## Design assets

                Curated reference + raw assets (brushed-metal faceplate, photoreal knob, VU meter reference, material panels, engraved/LED fonts, Fontaudio SVG icon set) feed the design-system generator.

                **Fonts:**
                - CF Engraved & Hultog Engraved (DaFont)
                - - Orbitron & Audiowide (Google Fonts, OFL)
                  - - Fontaudio (MIT)
                   
                    - ---

                    ## Roadmap

                    - [ ] Core JUCE plugin skeleton (VST3 + AUv3 targets)
                    - [ ] - [ ] Photoreal faceplate + knob rendering via Skia
                    - [ ] - [ ] Single-parameter DSP module (drive/saturation MVP)
                    - [ ] - [ ] Analog VU meter with ballistics
                    - [ ] - [ ] LED bypass indicator + click animations
                    - [ ] - [ ] Convolution engine (IR loading)
                    - [ ] - [ ] Neural model inference path
                    - [ ] - [ ] Preset browser + MIDI learn
                    - [ ] - [ ] Alt material variants (glass, marble, carbon fiber, crystal)
                   
                    - [ ] ---
                   
                    - [ ] ## Status
                   
                    - [ ] Private experimental project — personal use only, not for sale or distribution.
                   
                    - [ ] ---
                   
                    - [ ] ## License
                   
                    - [ ] All rights reserved. No license granted for use, modification, or redistribution.
                    - [ ] Third-party assets retain their original licenses (OFL, MIT, etc.) as noted above.
                    - [ ] 
