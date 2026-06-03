# WON-KNOBBER — RTNeural vs ONNX spike (integration-ready)

**Author:** Grok / WON Producer  
**Date:** 2026-06-02  
**Repo:** `~/Documents/GitHub/WON-KNOBBER` · CMake FetchContent JUCE 8.0.13, C++20, `-Werror`  
**Consumer:** Claude Code (`NeuralModel` wiring after IR PR)

Chain order (fixed): **saturation → convolution → neural** (`PluginProcessor.cpp`). `neuralModel.process` is commented until a model loads.

---

## Executive recommendation

| | Verdict |
|---|---------|
| **v1 ship** | **RTNeural** (BSD-3), **STL or XSIMD backend**, **fixed architecture** + weights in BinaryData (JSON export or compile-time `ModelT`) |
| **v1 defer** | **ONNX Runtime** — binary size, CI matrix, and dynamic linking cost outweigh benefit until user-supplied `.onnx` is a requirement |
| **Community NAM `.nam` full WaveNet** | **v1.1** — use [RTNeural-NAM](https://github.com/jatinchowdhury18/RTNeural-NAM) or [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) (MIT) after cab lane lands; not blocking Convolution PR |

---

## Comparison matrix

| Criterion | RTNeural | ONNX Runtime |
|-----------|----------|--------------|
| **License** | BSD-3-Clause ([RTNeural](https://github.com/jatinchowdhury18/RTNeural)) — **compatible** with PolyForm-NC plugin (separate work, attribution in NOTICES) | MIT ([onnxruntime](https://github.com/microsoft/onnxruntime)) — OK |
| **CMake / FetchContent** | `FetchContent_Declare(RTNeural …)` + `add_subdirectory`; set `RTNEURAL_STL=ON` to avoid Eigen submodule → **~header-only compile**, no extra dylib | Prebuilt **dynamic** libs per OS/arch (or heavy static build). Patterns: [ort-builder](https://github.com/olilarkin/ort-builder), [iPlug2OnnxRuntime](https://github.com/olilarkin/iPlug2OnnxRuntime) — **non-trivial** in our JUCE + macOS AU/VST3 + **Windows CI** matrix |
| **Build footprint** | +compile time (~few MB sources); **0 MB** runtime dylib if STL backend | **+15–80 MB** per platform in bundle or system ORT; codesign/notarization surface on macOS |
| **CI risk** | Low — same compiler flags as plugin | Medium/high — ORT version pin, Win vs mac ABI, optional GPU EPs off |
| **Model format** | JSON weights (TF/PyTorch export scripts in repo) or **compile-time `ModelT`** | `.onnx` arbitrary graphs |
| **NAM ecosystem** | Native path: train → export JSON; experimental `.nam` via RTNeural-NAM | NAM desktop exports ONNX in some tooling — second-class for RT-safe C++ without ORT |
| **RT-safe load** | Weights parsed on **message thread**; inference uses **preallocated** buffers in `prepare()` | `Ort::Session` created on message thread; IO tensors pre-bound in `prepare()` |
| **Typical latency (samples)** | **0** extra for feedforward Dense/tanh; **stateful** GRU/LSTM/WaveNet: algorithmic warm-up, report **0 PDC** for feedforward v1; WaveNet needs buffer delay analysis per architecture | Same — depends on graph; causal convs add delay = kernel size − 1 per stage unless compensated |
| **Integration cost (person-days)** | **2–4** — FetchContent, tiny model, swap, self-test | **5–10** — vendor ORT, CMake per platform, strip EPs, debug linker issues |

---

## RTNeural — build integration sketch

```cmake
# CMakeLists.txt (plugin repo) — additive
include(FetchContent)
FetchContent_Declare(
  RTNeural
  GIT_REPOSITORY https://github.com/jatinchowdhury18/RTNeural.git
  GIT_TAG        1fb1f075a5d66e85bfc8f488c3f3626840cb3a1d   # pinned in asset manifest 2026-06-02
  GIT_SHALLOW    TRUE
)
set(RTNEURAL_STL ON CACHE BOOL "" FORCE)  # smallest dep graph for CI
FetchContent_MakeAvailable(RTNeural)

target_link_libraries(WonKnobber PRIVATE RTNeural)
target_compile_definitions(WonKnobber PRIVATE RTNEURAL_DEFAULT_ALIGNMENT=16)
```

- **Do not** enable AVX flags unless CI targets guarantee it.  
- **Pin `GIT_TAG`** to a commit SHA when merging (reproducible builds).  
- Optional later: `RTNEURAL_XSIMD=ON` for Apple Silicon perf A/B.

**BinaryData model embed:**

```cmake
# underscore only
Resources/Models/neutral_color.json   # or .nam after v1.1
```

---

## RTNeural — RT-safe load story (required pattern)

Align with `CLAUDE.md` and existing `NeuralModel` stub:

### Message thread (editor open, preset load, factory voice change)

1. Read model bytes from `BinaryData` (never `juce::File` in release path for v1 embed).  
2. Parse JSON / construct `ModelT` on heap (**allocation OK here**).  
3. Build new instance in `std::unique_ptr<NeuralModelInstance> staging`.  
4. Call `staging->reset()` and dry-run one `forward()` to validate shape.  
5. **Atomic swap** `std::atomic<std::shared_ptr<NeuralModelInstance>> active` or double-buffer with `std::atomic<int> readIndex` (writer on message thread only, reader on audio thread).

### `prepare(sampleRate, blockSize)`

- Preallocate **input/output scratch** `juce::AudioBuffer<float>` or `std::vector<float>` sized to `blockSize * channels`.  
- No `resize` in `process()`.  
- If using stateful layers, call `model->reset()` on transport start / sample-rate change.

### `process(buffer)`

- `juce::ScopedNoDenormals`.  
- Per sample or per block: copy channel → `input[]` → `forward()` → copy out.  
- If `!active.load()`, pass-through (buffer unchanged).  
- **No** `new`, `malloc`, locks, file I/O, logging.

### Latency / PDC

- **v1 model:** 1×8 Dense + tanh (feedforward) → **0 samples** neural latency.  
- **Processor:** `setLatencySamples(saturation.getLatencySamples() + convolution.getLatency() + 0)`.  
- Recompute on IR load when convolution latency changes (JUCE `getLatency()`).

### Self-test (match WonKnobberState discipline)

On dylib load (static init in `NeuralModel.cpp` or shared test harness):

- Load embedded JSON from BinaryData.  
- `forward()` impulse {1,0,0…} → finite output, no NaN.  
- Log `NEURAL MODEL SELF-TEST: PASS/FAIL`.

---

## ONNX Runtime — why defer for v1

| Issue | Detail |
|-------|--------|
| **Dynamic library** | macOS AU/VST3 bundle must ship `libonnxruntime.dylib` or static link monster; Windows `.dll` + CI copy steps |
| **FetchContent** | ORT is not a single-header FetchContent like RTNeural; expect **vendored prebuilds** per arch |
| **Pluginval / hosts** | Extra dylib load paths break on some hosts if RPATH wrong |
| **Overkill** | v1 needs **one** bundled color model, not arbitrary user ONNX |

**When to revisit ONNX:** Phase 2c user model import, or mandatory compatibility with community ONNX exports without conversion to RTNeural JSON.

---

## Model format + license (v1 proposal)

| Asset | Format | License | Size (est.) |
|-------|--------|---------|-------------|
| **Neutral “color” stage** | RTNeural JSON (exported tiny MLP) | Model weights **your training** (no third-party weights) | 20–150 KB |
| **Optional v1.1** | `.nam` (NAM WaveNet) | MIT [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) + verify weight author | 100 KB–2 MB |

Training path (offline, not in plugin):

1. Capture/saturation chain training data from WON-KNOBBER saturation-only bounce.  
2. Train tiny PyTorch model (1–2 layers, <32 hidden).  
3. Export via RTNeural `python/model_utils.py` → JSON.  
4. Embed JSON in BinaryData.

**Do not** ship third-party NAM community weights without per-model license check.

---

## Per-option integration cost (for Claude Code scheduling)

| Option | Cost | Risk | v1 fit |
|--------|------|------|--------|
| **A — RTNeural tiny JSON embed** | **S** (2–4 d) | Low | **Best** |
| **B — RTNeural compile-time `ModelT`** | **S** (3–5 d) | Low | Best perf; less flexible |
| **C — NeuralAmpModelerCore + .nam** | **M** (5–8 d) | Med | v1.1 if “real amp model” required |
| **D — ONNX Runtime + .onnx embed** | **L** (8–12 d) | High | Defer |
| **E — ONNX user load from disk** | **L+** | High | Phase 2c only |

---

## Suggested PR sequence (Claude Code)

1. **PR: `feat/convolution-ir-embed`** — wire `Convolution` + BinaryData IRs + PDC (blocked on IR files from Grok/Kali — synthetic IR unblocks immediately).  
2. **PR: `feat/rtneural-neutral-color`** — FetchContent RTNeural, `NeuralModel` swap pattern, one JSON model, enable `neuralModel.process` in chain, extend `setLatencySamples`.  
3. **PR (optional): `feat/nam-wavenet-v1_1`** — after ear QA on saturation+cab+neutral stack.

---

## Grok deliverables (this spike)

- [x] This document  
- [x] IR shortlist (`WON_IR_cab_shortlist_spike_2026-06-02.md`)  
- [ ] Branch `feat/cab-ir-synth` with `cab_synth_neutral.wav` + LICENSE_IRS.txt (optional same-day)  
- [ ] Pin RTNeural commit SHA in draft CMake snippet when opening neural PR

---

## References

- [RTNeural](https://github.com/jatinchowdhury18/RTNeural) · BSD-3  
- [RTNeural-example](https://github.com/jatinchowdhury18/RTNeural-example) (JUCE plugin pattern)  
- [RTNeural-NAM](https://github.com/jatinchowdhury18/RTNeural-NAM) · MIT  
- [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) · MIT  
- JUCE `juce::dsp::Convolution::loadImpulseResponse(const void*, …)` + `getLatency()` — already in JUCE 8.0.13 deps