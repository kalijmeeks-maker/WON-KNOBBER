# CLAUDE.md

Working guide for Claude Code (and humans) on **WON-KNOBBER** — a photoreal single-knob
audio plugin (VST3 + AUv3) built with JUCE/C++.

## Project Purpose

One large photoreal rotary knob drives a single musically meaningful parameter (`drive`).
The plugin pairs an Acustica/UAD-style brushed-metal aesthetic with a real-time-safe DSP
core (tanh soft-clip MVP → convolution → optional neural inference). GUI is custom-drawn,
dark-mode only, using a filmstrip-PNG knob and an analog VU meter.

## Tech Stack

- **JUCE 7+** (audio plugin framework)
- **C++17** (language standard, enforced in CMake)
- **CMake** with `FetchContent` to pull JUCE — no submodule, no system install
- **Skia** intended for GPU-accelerated photoreal rendering (post-MVP); MVP uses JUCE Graphics
- **Formats:** VST3 (all platforms) + AU/AUv3 (macOS)

## Build Commands

CMake (primary, what CI uses):

    cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --config Debug

Release:

    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release -j

Projucer (optional/legacy): open a generated `.jucer` if one is added later. CMake is the
source of truth; do not hand-edit exporter files.

## Code Style

- 4-space indent, no tabs (`.editorconfig` + `.clang-format` enforce this)
- `#pragma once` for all headers (no include guards)
- Always qualify JUCE types as `juce::` — never `using namespace juce;` in headers
- Classes/structs: `PascalCase`; methods/functions: `camelCase`; member fields: trailing
  no-underscore camelCase, locals camelCase; constants in `ParamIDs` are lowercase string literals
- Allman braces, `ColumnLimit 120`, pointers left-aligned (`float* x`)
- Run `clang-format` before committing; CI assumes formatted code

## DSP Conventions (real-time safety)

`processBlock` runs on the audio thread. It MUST be real-time safe:

- **No heap allocation** (no `new`, `malloc`, growing `std::vector`, `juce::String`, etc.)
- **No locks** and **no blocking I/O** (no file/socket/log calls)
- Wrap the body in `juce::ScopedNoDenormals` for denormal protection
- Smooth every user-facing parameter with `juce::SmoothedValue<float>` (set in `prepareToPlay`)
- Allocate all buffers and load all assets (IRs, models) on the message thread, never in `processBlock`
- DSP modules expose a uniform contract: `prepare(double sampleRate, int blockSize)`,
  `process(juce::AudioBuffer<float>&)`, `reset()`

## GUI Conventions

- All custom drawing lives behind a `juce::LookAndFeel_V4` subclass (`KnobLookAndFeel`)
- Knob rendering uses a **filmstrip PNG**: a single tall image of **128 vertical frames**;
  `frameHeight = imageHeight / 128`; pick the frame from the normalized slider value
- Faceplate background is an SVG drawn behind the controls (`FaceplateView`)
- Load all image/font/SVG assets from `BinaryData` (compiled in via JUCE), never from disk at runtime
- Dark-mode only; colors come from the tokens documented in `docs/gui.md`

## Threading Model

Three threads, never share mutable state without care:

- **Audio thread** — `prepareToPlay`, `processBlock`, `releaseResources`. Real-time, lock-free.
- **Message (UI) thread** — editor construction, parameter changes from the UI, timers,
  asset loading. Safe to allocate here.
- **GPU render thread** — Skia/OpenGL context paint (post-MVP). Treat as message-thread-driven.

Pass values audio→UI via atomics or `juce::AudioProcessorValueTreeState`; never block the
audio thread waiting on the UI.

## Things to Never Do

- Never allocate, lock, or do I/O in `processBlock`
- Never load assets from the filesystem at audio time (use `BinaryData`)
- Never commit `Builds/`, `JuceLibraryCode/`, `build/`, or any compiled binary/plugin bundle
- Never write actual DSP math into the stub files until a dedicated `feat:` PR does so
- Never use `using namespace juce;` in a header
- Never add a `LICENSE` (repo is intentionally all-rights-reserved, private)

## How to Test Changes

1. Build with CMake (see above) — it must compile with **zero warnings** (CI fails on warnings).
2. Load the VST3/AU in JUCE's `AudioPluginHost` (see `.claude/commands/test-plugin.md`).
3. Sanity: turn `drive`, confirm no clicks/zipper noise (proves smoothing), confirm meter moves.
4. For DSP changes, verify real-time safety: no allocations in `processBlock` (profile if unsure).

## File-by-File Map

    CMakeLists.txt          JUCE FetchContent + juce_add_plugin (VST3, AU)
    Source/
      PluginProcessor.*     WonKnobberAudioProcessor; owns the `drive` parameter + DSP chain
      PluginEditor.*        WonKnobberAudioProcessorEditor; 480x180; hosts drive slider
      dsp/
        Saturation.*        tanh soft-clip stage (stub: prepare/process/reset/setDrive)
        Convolution.*       wraps juce::dsp::Convolution; loadIR()
        NeuralModel.*       placeholder neural inference; loadModel()
      gui/
        KnobLookAndFeel.*   LookAndFeel_V4; drawRotarySlider via filmstrip
        VUMeter.*           Component; 30Hz timer; amber->red gradient
        BypassLED.*         Component; setActive(bool); glow
        FaceplateView.*     root container; SVG faceplate; lays out knob/meter/LED
      util/
        Parameters.h        ParamIDs string constants
    Resources/              asset tree (filmstrips, faceplates, fonts, IRs, models) — see Resources/README.md
    docs/                   architecture, dsp, gui, conventions, glossary
    .claude/                Claude settings + slash commands
    .github/                CI workflow + issue/PR templates
