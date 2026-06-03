# WON-KNOBBER — Cab + neural integration spec (Grok → Claude Code / Design)

**Date:** 2026-06-02  
**Status:** Green-lit · runs parallel to Design rear-panel work · **does not touch front faceplate**  
**Machine-readable manifest:** [`WON_cab_neural_asset_manifest_2026-06-02.json`](./WON_cab_neural_asset_manifest_2026-06-02.json)

**Canonical repo:** `~/Documents/GitHub/WON-KNOBBER` (not `~/Projects/won-knobber` = Mastering Suite).

---

## 1. UI / product model (agreed — pending Kali confirm)

| Layer | Behavior |
|-------|----------|
| **Factory voice (8)** | Each preset carries **drive, mix, gem (variant), cabIr, neuralModel, cabEngage, neuralEngage** — no separate front-face cab/neural picker |
| **Rear panel** | User flips chassis → **override** IR well + neural well + engage rockers (Design anchors + PNG = long pole) |
| **Front face** | Unchanged (one hero knob + existing strip/transport) |

Grok **recommends Kali confirm** rear-fold + override (not front-face cab knobs) — avoids full faceplate re-spec.

---

## 2. Six IR assets (embed manifest)

All paths under `Resources/IRs/`. **Underscore filenames only** (JUCE BinaryData symbol sanitizer).

| ID | File | Mono | 48 kHz | Trim taps | ~Embed | License (v1) | Character |
|----|------|------|--------|-----------|--------|--------------|-----------|
| **FLAT** | `ir_flat.wav` | mono | yes | 2048 | 8 KB | Tier A in-house | Neutral / reference |
| **STUDIO RIBBON** | `ir_studio_ribbon.wav` | mono | yes | 2048 | 8 KB | Tier A in-house | Smooth, console |
| **VINTAGE 4×12** | `ir_vintage_4x12.wav` | mono | yes | 2048 | 8 KB | Tier A in-house | Classic 4×12 |
| **CONSOLE BOX** | `ir_console_box.wav` | mono | yes | 2048 | 8 KB | Tier A in-house | Tight box |
| **OLD RADIO** | `ir_old_radio.wav` | mono | yes | 1536 | 6 KB | Tier A in-house | Band-limited / lo-fi |
| **IRON CORE** | `ir_iron_core.wav` | mono | yes | 2048 | 8 KB | Tier A in-house | Aggressive mids |

**Total IR embed ≈ 46 KB** (negligible vs knob PNGs).

### License disqualifiers (do not embed)

- ML Sound Lab / retail IR EULAs (no redistribute)
- **CC-BY-SA**, **GPL**, no-redistribute packs
- True-stereo 2× length unless needed — v1 **mono only** to halve CPU and size

### Sourcing plan (Grok)

1. **v1 ship:** in-house capture or synthetic per row (PolyForm-NC safe).  
2. Optional reference: Soundwoofer PD IR — verify ToS; trim; one file only if needed.  
3. Design locks **per-voice → IR id** map (proposed table in JSON manifest until `SYNC_TO_CC_cab_neural_direction.md` lands).

---

## 3. Five neural slots

| ID | File | Embed | License | Latency (samples) |
|----|------|-------|---------|-------------------|
| **NONE** | — | 0 | bypass | 0 |
| **TAPE** | `model_tape.json` | ~64 KB | in-house weights + RTNeural BSD-3 notice | 0 (feedforward v1) |
| **VALVE** | `model_valve.json` | ~64 KB | in-house | 0 |
| **TRANSISTOR** | `model_transistor.json` | ~64 KB | in-house | 0 |
| **IRON** | `model_iron.json` | ~96 KB | in-house | 0 |

**Engine:** **RTNeural** (`RTNEURAL_STL=ON`, FetchContent, pin commit SHA). **Defer ONNX** — see [`WON_RTNeural_vs_ONNX_spike_2026-06-02.md`](./WON_RTNeural_vs_ONNX_spike_2026-06-02.md).

**Total neural embed ≈ 290 KB** (4 models; NONE has no file).

---

## 4. Hard constraint — RT-safe swap on preset load

Required: changing factory voice or rear-panel selection **must not** click, pop, or dropout.

### IR (JUCE `dsp::Convolution`)

- Load only on **message thread** from `BinaryData` pointer + size.  
- `loadImpulseResponse(const void*, size_t, Stereo::no, Trim::yes, 0, Normalise::yes)`.  
- JUCE queues IR swap — **wait-free on audio thread** (documented in `juce_Convolution.h`).  
- **Zero-latency mode (v1 default):** construct with `juce::dsp::Convolution()` (or `Convolution(Convolution::Latency{0})`). Preset-driven IR swaps must **not** churn `setLatencySamples` — some hosts glitch when PDC changes on every voice change. With zero-latency construction + trimmed mono IRs (≤2048 taps), `getLatency()` should stay **0** across all six cabs.  
- After load: `convLatency = convolution.getLatency()` → update processor PDC **once** at prepare; on swap, only call `setLatencySamples` if latency actually changed (should be rare in v1).  
- **Defer** `Convolution(Convolution::Latency{N})` with `N > 0` (partitioned / long-tail path) unless a specific cab truly needs extended tail **and** product accepts a one-time PDC bump for that IR only.

### Neural (RTNeural)

- Parse JSON / build model on **message thread** into `staging`.  
- Validate with one `forward()` on staging.  
- **Atomic swap** `std::atomic<std::shared_ptr<NeuralEngine>> active` (audio thread only reads `active`).  
- `prepare()`: all scratch buffers sized to `blockSize`; **zero alloc in `process()`**.

### Preset load path (Claude Code)

```
loadFactoryPreset(i) / applyState(st):
  1. applyStateToParams (drive, mix, variant, bypass)
  2. read cabIr, neuralModel, cabEngage, neuralEngage from state
  3. messageThreadAsync: load IR + swap neural (same call)
  4. recompute setLatencySamples
  5. editor: sync gem chip + faceplate (no new front cab UI)
```

**QA fallback:** if fast ‹ › cycling clicks, add dual conv instance with 32–128 sample crossfade (follow-up PR).

---

## 5. WonKnobberState schema extension (CC PR)

Append XML attributes only (never reorder existing fields):

```xml
<WonKnobberState version="1"
  drive="0.42" mix="1.0" variant="diamond" bypass="0"
  cabIr="STUDIO_RIBBON" neuralModel="TAPE" cabEngage="1" neuralEngage="1"/>
```

| Attribute | Type | Notes |
|-----------|------|-------|
| `cabIr` | string enum | One of six IDs |
| `neuralModel` | string enum | NONE \| TAPE \| VALVE \| TRANSISTOR \| IRON (Design locked — **not** `neural`) |
| `cabEngage` | 0/1 | Rear CABINET rocker |
| `neuralEngage` | 0/1 | Rear NEURAL rocker |

Update `toValueTree` / `fromValueTree`; extend factory XMLs (8 files); legacy reader defaults: `cabIr=FLAT`, `neuralModel=NONE`, engage=0 (CC PR1: off until cab/neural DSP ships).

---

## 6. Proposed 8-voice map (until Design locks)

| Idx | Voice | IR | Neural |
|-----|-------|-----|--------|
| 0 | TAPE HEAD | STUDIO RIBBON | TAPE |
| 1 | CONSOLE GLUE | CONSOLE BOX | TRANSISTOR |
| 2 | FURNACE | VINTAGE 4×12 | IRON |
| 3 | VELVET | FLAT | VALVE |
| 4 | SUNDAY DRIVE | OLD RADIO | TAPE |
| 5 | TUBE WARM | CONSOLE BOX | VALVE |
| 6 | DIODE BITE | IRON CORE | TRANSISTOR |
| 7 | TRANSFORMER | VINTAGE 4×12 | IRON |

Design: replace with authoritative map in `SYNC_TO_CC_cab_neural_direction.md`.

---

## 7. Claude Code PR sequence

1. `feat/won-state-cab-neural-fields` — schema + factory XML + tests (**in flight** — `neuralModel` attr)  
2. `feat/convolution-six-ir-embed` — wire `Convolution::process`, BinaryData, PDC (stack on **PR #40**)  
3. `feat/rtneural-five-slot-embed` — FetchContent RTNeural @ `1fb1f075a5d66e85bfc8f488c3f3626840cb3a1d`, `NeuralModel` swap, enable chain
4. `feat/rear-panel-ui` — **after** Design rear PNG + anchor JSON  

**Blocked on Grok assets for PR 2–3:** WAV/JSON bytes in `Resources/`. Grok will open `feat/cab-neural-assets` branch with **synthetic IR placeholders** so PR 2 can start before mic captures.

**Not blocked:** PR #38 bypass + About (in review).

---

## 8. Design dependencies (not Grok)

| Item | Owner | Blocks |
|------|-------|--------|
| `SYNC_TO_CC_cab_neural_direction.md` + visual board on CC disk | Design → commit `docs/` or design repo | About wording parity; final voice map |
| Rear 960×600 anchor JSON + brushed PNG | Design | Rear UI PR |
| Per-voice cab+neural map (authoritative) | Design | Factory XML final values |

**Note:** `SYNC_TO_CC_cab_neural_direction.md` was **not found** on disk (2026-06-02) — only older `SYNC_TO_CC_filmstrip_answers.md` / `gem_decision` in Downloads handoff.

---

## 9. Grok deliverables checklist

- [x] Integration spec (this file)  
- [x] JSON asset manifest  
- [x] IR shortlist + RTNeural spike (prior uploads)  
- [x] Branch `feat/cab-neural-assets-manifest` — 6 distinct synthetic WAV + 4 stub JSON (**PR #40** tracking)  
- [x] `Resources/IRs/LICENSE_IRS.txt` + `Resources/Models/LICENSE_MODELS.txt` (plugin branch)  
- [x] Tier A in-house IR captures (2026-06-02) — `Scripts/generate_cab_ir_captures.py`, manifest tap counts (**PR #40**)  
- [ ] Optional: replace with mic'd cab captures (same filenames — no CC code change)

---

## 10. Paste-ready → Claude Code

```
▎ Grok: cab/neural asset manifest landed — design repo uploads/WON_cab_neural_asset_manifest_2026-06-02.json
▎ + WON_cab_neural_integration_spec_2026-06-02.md. Six IRs + five neural slots + RT-safe swap contract +
▎ WonKnobberState v2 attrs + proposed 8-voice map. RTNeural v1 / ONNX deferred. Front face untouched;
▎ preset-driven + rear override. Wire against manifest filenames/symbols exactly. Placeholder asset branch next.
```