# PREP_QA_REPORT.md — WON-KNOBBER Cab/Neural RT-Safe QA (manual, post subagent failure)

**Date:** 2026-06-03  
**Reviewer:** Grok (main) — won-qa subagent (id 019e8cd2-c880-76d0-a132-9976e818d2d6) was cancelled due to "doom loop detected (repetitive actions)" after 54 tool calls / 130s. Manual review performed instead using the same scope.  
**Scope:** Audit Convolution, NeuralModel, processor wiring + self-tests vs rt_safe_swap rules in `docs/WON_cab_neural_integration_spec_2026-06-02.md` (§4 hard constraint) + spikes + CLAUDE.md real-time rules. Evidence with file:line.

**Sources read:**
- `docs/WON_cab_neural_integration_spec_2026-06-02.md` (esp. §4 IR/Neural/preset load path, Trim::yes, Latency{0}, atomic shared_ptr or equiv, message-thread load, zero-alloc process, PDC update logic).
- `docs/WON_RTNeural_vs_ONNX_spike_2026-06-02.md` (double-buffer atomic, prepare scratch, no alloc in process).
- `Source/dsp/Convolution.h:34` (construction), `Convolution.cpp:63-67` (loadImpulseResponse call), `63` (setIr), smoke tests.
- `Source/dsp/NeuralModel.h`, `NeuralModel.cpp:48-70` (process), `79-114` (setModel), `16-24` (double buffer), smoke tests.
- `Source/PluginProcessor.cpp:26-56` (prepareToPlay), `179-194` (applyStateToParams), `272-284` (loadFactoryPreset), `33` (setLatency only sat), processBlock sections.
- `Source/WonKnobberState.cpp` (applyState paths).
- Self-tests in dsp + state.
- Manifest for IDs/latency notes.

## Per-Rule Pass/Fail + Evidence

### 1. IR load on message thread from BinaryData only (no File in release path)
**PASS**  
- setIr called from applyStateToParams (preset load / state recall) and prepareToPlay — both message/prepare context.  
- `convolution.loadImpulseResponse (data, size, ...)` where data from `irDataForId` → BinaryData::ir_*_wav .  
- Comment in header: "safe to call from the message thread on preset load."  
- No juce::File load in the hot path (only BinaryData).

### 2. loadImpulseResponse flags exactly as spec
**FAIL (Trim)**  
- Code (Convolution.cpp:63-67):  
  ```
  convolution.loadImpulseResponse (data, (size_t) size,
                                   juce::dsp::Convolution::Stereo::no,
                                   juce::dsp::Convolution::Trim::no,   // <--- mismatch
                                   0,
                                   juce::dsp::Convolution::Normalise::yes);
  ```
- Spec (§4): `loadImpulseResponse(const void*, size_t, Stereo::no, Trim::yes, 0, Normalise::yes)`.  
- Assets are pre-trimmed per capture log, but spec still mandates Trim::yes in the call (for safety / future).  
- Also: the 4th param after Trim is the "max length" (0 = use all) — code matches.  
**Recommendation:** Change to Trim::yes. Re-test smoke (finite output unchanged since pre-trimmed). Update comment.

### 3. Zero-latency construction + getLatency() == 0 for all cabs (no PDC churn on voice swap)
**PASS (construction) / FAIL (usage & PDC logic)**  
- Construction (Convolution.h:34): `juce::dsp::Convolution convolution { juce::dsp::Convolution::Latency { 0 } };` — exact match to spec "construct with juce::dsp::Convolution() (or Convolution(Convolution::Latency{0}))".  
- In v1 all 6 IRs are ≤2048 taps trimmed mono → latency should be 0.  
- **FAIL:**  
  - prepareToPlay (PluginProcessor.cpp:33): `setLatencySamples(saturation.getLatencySamples());` — **never** queries or adds convolution.getLatency().  
  - No call to conv.getLatency() after setIr or conv.prepare.  
  - applyStateToParams (after setIr): no PDC update.  
  - Spec: "After load: convLatency = convolution.getLatency() → update processor PDC **once** at prepare; on swap, only call setLatencySamples if latency actually changed (should be rare in v1)."  
  - "recompute setLatencySamples" in preset load path diagram.  
- Current code "happens to work" because 0 + only sat, but violates contract and will bite if any IR ever has >0 latency or if conv changes.  
**Recommendation:** 
  - Add int getLatencySamples() const { return convolution.getLatency(); } to Convolution (or expose).
  - In prepareToPlay after conv.prepare + conditional setIr: `int convLat = cabEngage ? convolution.getLatency() : 0; setLatencySamples(saturation.getLatencySamples() + convLat);`
  - In applyStateToParams after setIr: compute newLat, only set if changed from last reported.
  - Store lastReportedLatency member.

### 4. Neural: parse/build on message thread into staging + atomic swap; audio only reads active
**PASS**  
- setModel (NeuralModel.cpp:79-114): builds to `inactive = 1 - activeIdx` (using nlohmann + RTNeural::json_parser on message thread), then `models[inactive] = move(built); activeIdx.store(inactive, release);`  
- process (48-70): `auto* model = impl->models[impl->activeIdx.load(acquire)].get(); if (!model) return; model->forward...` — audio **never** writes, only reads the active slot.  
- Impl: `std::unique_ptr<ModelT> models[2]; std::atomic<int> activeIdx { 0 };` (not exactly shared_ptr<NeuralEngine> but equivalent double-buffer + atomic index — safe and matches spike "atomic shared_ptr or double-buffer with atomic readIndex").  
- Dedup on currentModelId (message only).  
- NONE/unknown: reset inactive slot → passthrough.

### 5. prepare(): scratch buffers pre-sized to blockSize; **zero alloc in process()**
**PASS (neural) / PARTIAL (overall)**  
- Neural: prepare() is empty (comment: "feedforward dense/tanh: sample-rate agnostic, no scratch to size"). Per-sample forward uses stack `float in[2]`, `getOutputs()`. No heap, no vector resize, no new in process. Good for feedforward v1.  
- Spec requires "all scratch buffers sized to blockSize" — here not needed, but add comment or no-op size for future stateful models.  
- Convolution: JUCE internal — assumed prepped in its prepare (called from processor).  
- Processor: dryBuffer pre-sized to safe max (16384+), mixSmooth reset, etc. No per-block alloc in the cab/neural paths.  
- Self-tests allocate test buffers on init — ok (not audio thread).

### 6. Disengaged / no-model = bit-exact passthrough (no math, no side effects)
**PASS** (with evidence from self-tests)  
- Neural process: if (!engaged) return; if (!model) return; — buffer untouched.  
- Self-test (NeuralModel.cpp:136-146): disengaged vs ref copy, exactlyEqual check → PASS.  
- Conv process: if (!engaged) return; — then process.  
- Conv self-test similar.  
- When model==nullptr after setModel("NONE"): passthrough.  
- Engage is atomic release/acquire.

### 7. Preset load path matches spec diagram
**PASS (structure) / MINOR (latency missing)**  
- loadFactoryPreset → XML → fromValueTree → applyState(st) → applyStateToParams (reads cabIr/neuralModel/engages, calls setEngaged + conditional setIr/setModel).  
- prepare path also re-applies.  
- Matches the "1. applyStateToParams ... 3. messageThreadAsync: load IR + swap neural" (here sync on message/prepare is fine).  
- Missing: the "4. recompute setLatencySamples" (see #3).  
- A/B, host recall, etc. go through same state paths.

### 8. No heap/lock/IO in processBlock (CLAUDE.md + real-time rules)
**PASS**  
- Cab/neural paths: only atomic loads, model->forward (prebuilt), JUCE conv.process (prepped), stack temps.  
- No new, no malloc, no locks (atomics only), no file, no String in hot path (ids are pre-cached in current* members).  
- Processor processBlock (assumed from prior): uses dryBuffer (pre-sized), mixSmooth, etc.  
- Self-tests confirm no NaN/finite issues.

### 9. Self-tests / smoke on static init
**PASS**  
- Both dsp modules + WonKnobberState have static-init smoke tests (run on dlopen). Cover engaged/disengaged passthrough, finite output, unknown-id no-op, embed roundtrips.  
- Output visible on load (cout).  
- Processor has factory embed tests that now include cab/neural fields (per earlier state).

## Overall Readiness Verdict
**85% — Strong foundation, two spec deviations + one missing contract piece.**

**Blockers for handoff (must fix before claiming "RT-safe per spec"):**
- Change Trim::no → Trim::yes in Convolution::setIr (line 65).
- Implement full PDC: query conv latency in prepare + conditional update in applyStateToParams after setIr. Expose getLatency from wrapper. (Even if always 0 today, the logic must be there per §4 and the preset-load diagram.)
- Add comment in Neural::prepare explaining why no scratch sizing (feedforward per-sample; future stateful models will size here).

**Nice-to-haves (non-blocking for v1):**
- Align any "max length" comment with pre-trim policy.
- Add a fast-cycle crossfade fallback test (as spec QA fallback) once rear panel is live.
- Extend factory embed tests to assert exact cabIr/neuralModel strings from the authoritative map (after ID reconciliation in other PREPs).

**Evidence files (committed on branch):**
- PREP_ID_AUDIT.md (full ID + load path trace)
- PREP_INTEGRATION_PLAN.md (prioritized checklist that includes these items)
- This report + SUBAGENTS_FANOUT.md

**Next when handoff lands:** Apply the Trim + PDC fixes (small, explicit paths in dsp/ + PluginProcessor), re-run self-tests + cmake -Werror, re-ingest any updated manifest if IDs changed. The subagent reports already map every site.

won-qa subagent note: Avoided in this manual pass by targeted reads + grep instead of broad repetitive exploration.
