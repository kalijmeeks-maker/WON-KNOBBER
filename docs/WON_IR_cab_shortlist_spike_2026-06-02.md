# WON-KNOBBER — IR cab shortlist spike (integration-ready)

**Author:** Grok / WON Producer  
**Date:** 2026-06-02  
**Repo:** `~/Documents/GitHub/WON-KNOBBER` (canonical) · main @ ac1730c+  
**Consumer:** Claude Code (Convolution wiring PR) · Claude Design (cab-voice direction)

PolyForm Noncommercial plugin (`LICENSE`). Third-party IRs must be **redistributable inside the plugin binary** (BinaryData embed). No disk load at audio time (`CLAUDE.md`).

---

## v1 integration constraints (from CLAUDE.md)

| Rule | Implication |
|------|-------------|
| Embed via `juce_add_binary_data` | IRs live under `Resources/IRs/`; **underscore filenames only** (`cab_tight_4x12.wav` → `BinaryData::cab_tight_4x12_wav`) |
| No filesystem at runtime | `Convolution::loadIR` should decode from `BinaryData` pointer + size → `juce::dsp::Convolution::loadImpulseResponse(const void*, size_t, ...)` |
| Message-thread load | Build `AudioBuffer<float>` or pass WAV bytes on UI thread; JUCE queue swaps IR (see `juce_Convolution.h` — load is wait-free on audio thread) |
| PDC | After load, read `convolution.getLatency()`; processor must set **`setLatencySamples(saturation + conv + neural)`** (today only saturation is wired) |
| Target format | **48 kHz** reference; mono or true-stereo; **trimmed** length (see below) |

**CPU / length guidance:** JUCE partitioned convolution cost scales with IR length. For v1, **trim to 512–2048 samples** (~10–43 ms @ 48 kHz) after `Trim::yes`. Long retail IRs (e.g. 200 ms / 9600 taps) are **disqualified for embed** unless heavily trimmed.

**Bundle budget (v1 proposal):** 3 mono IRs × 2048 taps × 4 bytes ≈ **24 KB** raw PCM in BinaryData (acceptable vs ~43 MB knob filmstrips).

---

## Shortlist

### ✅ RECOMMENDED — Tier A (ship v1)

#### A1 — In-house captures (3 voices, primary)

| Field | Value |
|-------|--------|
| **License** | You own the recordings → **PolyForm-NC–safe** embed; no third-party EULA |
| **Format** | WAV 48 kHz, 24-bit capture → embed as mono float in BinaryData |
| **Length** | Record ~100–200 ms; **trim to 1024–2048 taps** for production embed |
| **Channels** | Mono (SM57-style spot) per cab; stereo optional later |
| **Embed plan** | `Resources/IRs/cab_tight_4x12_sm57.wav`, `cab_open_2x12.wav`, `cab_dark_1x12.wav` (~8 KB each trimmed) |
| **Cab character (Design input)** | **tight_4x12** — punchy mids, controlled low; **open_2x12** — airy, less box; **dark_1x12** — softer top, thicker low-mid |
| **Integration cost** | Low — drop files + CMake list + `loadImpulseResponse` from BinaryData |

**Action for Kali/Design:** Pick 3 physical cab *characters* aligned to factory voice families (e.g. FURNACE → tight_4x12, VELVET → open_2x12, TUBE WARM → dark_1x12). One mic position per IR for v1.

---

#### A2 — Soundwoofer “Rocksta Fender Twin Reverb SM57” (single reference IR)

| Field | Value |
|-------|--------|
| **Source** | [Soundwoofer impulse](https://soundwoofer.com/Impulse/Index/812e25aa-c517-44f9-9cea-23c4f03a72ac) — cited as **public domain** via [jfsantos/guitar-fx `data/ir.wav`](https://github.com/jfsantos/guitar-fx) |
| **License** | **Likely OK** for NC redistribution if Soundwoofer PD claim holds — **verify Soundwoofer ToS once before merge**; keep attribution line in `Resources/IRs/LICENSE_IRS.txt` |
| **Format** | WAV; resample to **48 kHz**; **mono** for v1 embed |
| **Length** | Download full file → measure taps → **trim to ≤2048** for embed |
| **Channels** | Mono embed |
| **Embed plan** | `Resources/IRs/cab_fender_twin_sm57.wav` (~8 KB trimmed) |
| **Cab character** | Clean American open back / twin — good “CONSOLE GLUE” / “SUNDAY DRIVE” anchor |
| **Integration cost** | Low (same as A1) |

---

#### A3 — Synthetic minimum-phase IR (fallback, zero legal risk)

| Field | Value |
|-------|--------|
| **License** | Generated in-repo (e.g. exponential decay + resonant peaks script, MIT tooling) |
| **Format** | 48 kHz mono WAV, 512–1024 taps |
| **Embed plan** | `cab_synth_neutral.wav` (~4 KB) |
| **Cab character** | Neutral combing / damped — useful as “default cab” until real captures land |
| **Integration cost** | Low + one-time generator script (not in audio path) |

---

### ⚠️ CONDITIONAL — Tier B (needs written permission or legal read)

| Candidate | Blocker |
|-----------|---------|
| **ML Sound Lab “Free Premium IR”** | EULA: *“Reproduce, copy, distribute, resell or otherwise use the products for any commercial purpose”* — **embedding in a distributed plugin is redistribution of the WAV** → **DISQUALIFIED** without ML Sound Lab written embed grant |
| **PreSonus / Wilkinson / Origin / ML paid packs** | Retail EULA; no embed rights → **DISQUALIFIED** |
| **OwnHammer / Celestion / 3rd-party commercial libraries** | No redistribution → **DISQUALIFIED** |

---

### ❌ DISQUALIFIED — Tier C (do not embed)

| Candidate | Reason |
|-----------|--------|
| **CC-BY-SA** IRs | Share-alike may conflict with combined NC binary; legal review required → treat as **DISQUALIFIED** for v1 |
| **GPL** IR collections | GPL on assets → **DISQUALIFIED** |
| **“Free download” packs with no redistribution clause** | Personal-use only → **DISQUALIFIED** |
| **User-supplied IR folder at runtime** | Violates BinaryData rule for v1 (Phase 2c file I/O only) |

---

## Recommended v1 bank (for Claude Code wiring)

| Index | BinaryData name | Voice / preset alignment (starting point) |
|-------|-----------------|-------------------------------------------|
| 0 | `cab_tight_4x12_sm57` | FURNACE, DIODE BITE, TRANSFORMER |
| 1 | `cab_open_2x12` | VELVET, CONSOLE GLUE, TAPE HEAD |
| 2 | `cab_dark_1x12` | TUBE WARM, SUNDAY DRIVE |

Factory preset XML already sets drive/mix/gem; **cab selection** can be:

- **Preset-driven (no new knobs):** map each of 8 factory XMLs to `cabIndex` in processor (Design controls call deferred).
- **Single default cab** until Design picks visible vs menu-driven UX.

---

## Wiring checklist (Convolution PR — Claude Code)

1. Add IR paths to `juce_add_binary_data` in `CMakeLists.txt` (underscore names).
2. Replace `Convolution::loadIR(File)` stub with `loadIRFromMemory(const void* data, size_t size)` calling:
   ```cpp
   convolution.loadImpulseResponse(data, size,
       juce::dsp::Convolution::Stereo::no,  // mono IR
       juce::dsp::Convolution::Trim::yes,
       0,  // 0 = use trimmed length cap per JUCE
       juce::dsp::Convolution::Normalise::yes);
   ```
3. On successful load (message thread): `convLatency = convolution.getLatency();` notify processor to recompute `setLatencySamples(saturation.getLatencySamples() + convLatency + neuralLatency)`.
4. Implement `process()` with `juce::dsp::AudioBlock` + `juce::dsp::ProcessContextReplacing` (pattern in JUCE Convolution demo / tests).
5. Add `Resources/IRs/LICENSE_IRS.txt` per asset (source URL + license line).
6. Self-test on dylib load: parse each embedded WAV, load IR, assert `getLatency() >= 0`, optional impulse peak sanity.

---

## Design deliverable (cab-voice direction)

Use the **Tier A character notes** above in DESIGN_DIRECTION §1 when mapping 8 factory voices to cab indices. No faceplate change required for preset-driven cab mapping.

---

## Grok next actions (assets)

1. Generate **A3** synthetic neutral IR (same day, zero blockers).  
2. Download **A2**, measure taps/KB, trim, park under `Resources/IRs/` in a `feat/cab-ir-embed` branch for Claude Code.  
3. Coordinate **A1** captures with Kali (3 filenames above).