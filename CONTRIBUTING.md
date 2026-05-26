# Contributing to WON-KNOBBER

Private project. These conventions keep history clean and the audio thread safe.

## Branch Naming

Use a type prefix matching the work:

- `feat/<slug>` — new functionality
- `fix/<slug>` — bug fix
- `chore/<slug>` — tooling, scaffolding, config
- `docs/<slug>` — documentation only

Example: `feat/convolution-ir-loading`, `fix/zipper-noise-on-drive`.

## Conventional Commits

Every commit message follows:

    <type>(<optional scope>): <description>

Types: `feat`, `fix`, `docs`, `chore`, `build`, `ci`, `refactor`, `test`, `perf`.
Keep the subject imperative and under ~72 chars. Group one concern per commit.

Examples:

    docs: add CLAUDE.md working guide
    feat: add stub source tree
    ci: add cross-platform build workflow

## PR Checklist

- [ ] Builds with `cmake --build build` on macOS and Windows (CI green)
- [ ] Zero compiler warnings
- [ ] `clang-format` applied
- [ ] No allocations / locks / I/O added to `processBlock`
- [ ] No binaries, `build/`, `Builds/`, or `JuceLibraryCode/` committed
- [ ] Docs updated if behavior or conventions changed
- [ ] PR uses the template and lists DAWs tested

## Local Build Verification

    cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --config Debug
    # then load the built VST3/AU in JUCE AudioPluginHost

## File Header Template

    /*
        <FileName> — <one-line responsibility>
        WON-KNOBBER · part of the <dsp|gui|core> layer
    */
    #pragma once
