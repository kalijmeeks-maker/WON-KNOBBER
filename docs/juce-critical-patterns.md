# JUCE Critical Patterns — Required Reading

## RT-safety (processBlock is sacred)
1. No heap allocation in processBlock — pre-allocate in prepareToPlay.
2. No locks, no mutexes — use lock-free FIFO / atomics for thread comms.
3. No logging, no file I/O, no JUCE String construction on the audio thread.
4. Denormals: ScopedNoDenormals at the top of processBlock.
5. Oversampling buffers allocated in prepareToPlay sized for the 32x max,
   then index down to 16x for realtime.

## GUI / painting (kill the jank)
6. Never allocate in paint(). Cache Paths, Images, Colours as members.
7. Cache the faceplate background as an Image; don't re-decode SVG per paint.
8. Filmstrip knob: load the 120-frame strip ONCE; blit the frame, never redraw.
9. Use setBufferedToImage() for static layers; repaint only dirty regions.
10. VBlankAttachment / friz for animation timing — not Timer at 60Hz.
11. melatonin_inspector in debug builds to catch overdraw and layout drift.

## Layout
12. place(x,y,w,h) scales from kRefW x kRefH against the plate-truth JSON.
13. resized() reads anchors from JSON only — zero literal coordinates.

## State
14. Magic-header XML blob is versioned and decoupled from the param list.
15. New params append with safe defaults; old sessions load without breakage.
16. setStateInformation must tolerate missing keys (forward/backward compat).

## Host correctness
17. beginChangeGesture/endChangeGesture wrap every automatable param move.
18. Report latency (setLatencySamples) for oversampling + convolution PDC.
19. Bypass is a real param honoring PDC, not a UI-only toggle.

---

# Detailed patterns — WON-KNOBBER specifics + code examples

> Canonical detail folded in from the v2 reference (richer than the 19-rule quick-ref above).


**This is auto-injected into every subagent BEFORE any JUCE codegen.** It exists to stop
models emitting deprecated / broken JUCE patterns and to encode the stack-specific rules that
prevent the whole v1 class of bug.

Scope lock: **macOS only · AUv3 + VST3 only · native C++ UI · JUCE 8.0.x** (track 9 preview).
CoreGraphics is the sole render target (Metal/GPU optional later). No Windows, no WebView,
no AUv2/AAX. Anything flagged `(verify)` is an unverified community/CPU figure — confirm before
betting on it.

Pair with `V2_BRIEF.md` (product/engineering spec) and `DESIGN_SYSTEM.md` (visual + interaction
rulebook). Those two own the *what*; this file owns the *how-to-write-the-code-safely*.

---

## I. Foundation rules — the non-negotiables

### 1. `processBlock` is RT-safe: treat it like a hard real-time contract
The audio thread MUST NEVER:
- `new` / `malloc` / grow a `std::vector` (`push_back`/`reserve`/`resize`-with-growth)
- take any lock (mutex, spinlock, `CriticalSection`)
- do blocking I/O (file / socket / print / log)
- construct or mutate a `juce::String` (each is an allocation = RT death)
- call `repaint()` or touch a `Component`

```cpp
void WonKnobberAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals denormals;          // always, first line
    const int numSamples = buffer.getNumSamples();

    // 1) update smoothers/targets from atomic param loads (no locks)
    mixSmooth.setTargetValue(mix->load(std::memory_order_relaxed));

    // 2) run the linear DSP chain; each stage is RT-safe by contract
    saturation.process(buffer);
    if (convolution.isEngaged())  convolution.process(buffer);
    if (neuralModel.isEngaged())  neuralModel.process(buffer);
    dryWet.applyCrossfade(mixSmooth, buffer /*wet*/, dryBuffer);
}
```

### 2. Every UI-facing parameter is a `SmoothedValue` (no zipper noise)
Smoothing is the ONLY safe place to coerce parameter values on the audio thread.
- Message thread: `mixSmooth.setTargetValue(newValue)`.
- Audio thread: `mixSmooth.getNextValue()` per sample inside the DSP stage.
- `reset(sampleRate, 0.015)` (10–20 ms ramp) in `prepareToPlay`.
- The internal target read/write is atomic + lock-free; a concurrent set/get yields a
  consistent (slightly stale) value, never corruption.
- Do NOT hand-roll your own lerp on the audio thread — you lose atomicity or add a block of latency.

### 3. Load all buffers / IRs / models on the message thread, swap via lock-free handoff
Assets (IR WAVs, neural JSON/weights, tables) are NEVER loaded in `processBlock`.

```cpp
// message thread (GUI or preset load)
void Convolution::setIr(const juce::String& irId) {
    auto next = std::make_unique<juce::dsp::Convolution>();
    next->loadImpulseResponse(...);                 // background-threaded inside JUCE
    irStorage.store(next.release(), std::memory_order_release);
}
// audio thread
auto* ir = irStorage.load(std::memory_order_acquire);
if (ir) ir->process(...);
```

### 4. No singletons / statics / thread_local for mutable state — multi-instance safety
Hosts load many instances of one `.vst3` / `.appex` into one session and **may share the audio
thread**. You cannot know from inside the plugin. Therefore:
- All state lives on the **processor instance** (constructed in ctor + `prepareToPlay`).
- No mutable `static`, no `getInstance()` singleton, no `thread_local` (two instances can share
  one thread, so `thread_local` is NOT an escape).
- Pass state down the component/DSP hierarchy; each module owns its own buffers + atomics.

**Immutable read-only statics ARE fine** — a `static const juce::StringArray` lookup table
(gem-stone names, cab-IR IDs) is constructed once and only read; concurrent reads across instances
and threads cannot race. Mutable statics (caches, pools) are the violation.

### 5. `parameterValueChanged()` can fire on the AUDIO thread (poorly documented)
`AudioProcessorParameter::Listener::parameterValueChanged()` runs on **whatever thread wrote the
value**. In VST3 with sample-accurate automation that is often the audio thread, even though many
DAWs call it on the message thread. The only safe body is a single lock-free atomic store.

```cpp
void WonKnobberAudioProcessor::parameterValueChanged(int, float) {
    if (bypassParam) bypassState.store(bypassParam->get(), std::memory_order_relaxed); // RT-safe
}
```
NEVER inside it: `repaint()`, allocation, locks, I/O, buffer resize. Defer heavy work to the UI
thread via a `VBlankAttachment` poll or `MessageManager::callAsync`.

### 6. Never `repaint()` from the audio thread
A `repaint()` queues an invalidation; called from the audio thread it can stall the message loop
→ glitch/dropout. UI reads audio state via **atomics**, polled on a UI-thread timer or
`VBlankAttachment`. Batch updates; never repaint per sample.

---

## II. Animation backbone — vsync-locked, frame-rate agnostic

### 7. Prefer `juce::VBlankAttachment` over naive `Timer` for animated repaints
A `startTimerHz(N)` assumes a fixed frame rate: a 60 Hz flip plays at effective 60 fps on a
120 Hz display (stutter), and fires late under DSP load (dropped frames). `VBlankAttachment`
(JUCE 7.0.6+) binds the repaint to the display vsync — on macOS it wraps `CADisplayLink`, so you
get vsync-locked updates for free with no OpenGL setup. CoreGraphics respects vsync natively.

```cpp
class FlipTransition : public juce::Component {
    juce::VBlankAttachment vblank { getParentComponent(), [this]{ onVBlank(); } };
    void onVBlank() {
        if (!animating) return;
        const double elapsed = juce::Time::getMillisecondCounterHiRes() - startMs;
        if (elapsed >= kDurationMs) { animating = false; repaint(); fireDone(); return; }
        repaint();                                  // paints on the next vsync, not a timer
    }
};
```
Adopt for: the gem/drive/mix knobs (on param change), `IOMeter` ballistics, `FlipTransition`
(450 ms duration stays constant at 60 Hz and 120 Hz), and the PWR/SIG/CLIP status LEDs.

**Gotchas:** the attachment needs a non-null `getParentComponent()` — construct it lazily
(in `resized()` / after `addAndMakeVisible`). The callback must not allocate or block. JUCE does
not expose the monitor Hz — infer it from the interval between two callbacks if you need it.
Migrating from a slow `startTimerHz(N)` *increases* repaint frequency to the true display Hz, so
profile any expensive paint first. Honor macOS "Reduce motion": skip the tween, set final state.

### 8. Meter ballistics are TIME-based (`dt`), never frame-based
`IOMeter::advanceRow()` already uses `float dt` (seconds since last frame) for instant-rise /
linear-decay — keep it. When migrating from the fixed 30 Hz timer to `VBlankAttachment`, compute
`dt` from the actual interval between callbacks and **clamp** it (e.g. `jlimit(0.0, 0.25, …)`) to
guard against a stalled message thread.

```cpp
const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
const float  dt  = lastTickSec > 0.0 ? (float) juce::jlimit(0.0, 0.25, now - lastTickSec)
                                     : 1.0f / 30.0f;
lastTickSec = now;
faceplate.pushLevels(p.inL, p.inR, p.outL, p.outR, dt);
```
Never `bar -= 0.01 per frame` — it breaks under frame-rate jitter or CPU load.

### 9. Optional motion libraries (NOT v2 MVP requirements)
- **friz 2.0** (bgporter, MIT) — built on `VBlankAttachment`; millisecond-duration value tweens
  with easing. Adopt only if a future motorized-knob tween or a pulsing CLIP-LED glow is added.
  Current knobs jump to the param value; no tween needed for MVP.
- **rLottie** (chrisboy2000 / HISE, MIT) — renders Bodymovin (After Effects) JSON vector
  animations. Use ONLY for vector overlays on top of the baked plate (glow rings, loading
  spinners). NEVER for the gem knob — keep that a fast baked 120-frame filmstrip; rLottie is
  slower and loses the photoreal finish.

---

## III. Painting performance — eliminate jank

### 10. Paint-as-atomic: pre-allocate paths/gradients as members, rebuild on change only
`paint(Graphics&)` runs on the message/render thread; treat it like `processBlock` — no
allocation, no hidden work. Hold heavyweight `juce::Path` / gradient objects as **members**;
rebuild them only when the driving value changes (coarse-grained), not every frame.

```cpp
private:
    juce::Path curvePath;            // allocated once
    bool needsPathRebuild { true };

void setDrive(float d) {
    if (std::abs(d - drive) > 1.0e-4f) { drive = d; needsPathRebuild = true; repaint(); }
}
void paint(juce::Graphics& g) override {
    if (needsPathRebuild) rebuildPath();
    g.strokePath(curvePath, stroke);
}
```
Applies to `TransferCurve`, `HarmonicBars`, and the `KnobLookAndFeel` value-arc — none of these
should rebuild a `Path` inside `paint()` at 30 Hz.

### 11. Opaque + buffered: lock the static faceplate layer away from live overlays
The GUI is layered: static baked faceplate (960×612 PNG) → live overlays (knob/meters/curve/flip)
→ modal scrim. Any child `repaint()` cascades into a parent repaint that re-blits the whole PNG.
Break the chain:

1. Put the faceplate + footer bay in their own `FaceplateBackground` child component.
2. `setOpaque(true)` on it — signals "no transparency; children don't need me repainted."
3. If profiling still shows it hot at rest, `setBufferedToImage(true)` — cache to an off-screen
   image, blit each frame, re-rasterize only on resize.

Reported win: ~5–15% off the GUI thread by separating static from dynamic layers `(verify)`.

### 12. Modals / scrims are CHILD components, not `paintOverChildren()` work
`paintOverChildren()` runs after every child paints, so any live child's `repaint()` drags the
parent (and the static faceplate PNG) into a redraw. Extract the About/Licences modal and the
bypass wash into their own `addChildComponent` layers (`setVisible(true/false)` to toggle). Their
`paint()` runs independently; the faceplate never re-rasterizes when the modal opens or bypass
toggles.

### 13. Cache the static value-arc; rebuild only the live (filled) arc
The hero knob's amber value-arc has a **constant** track (radius + start/end angle) and a
**variable** fill (`toAngle`). Cache the track `Path` keyed on knob size; rebuild only the small
fill arc per frame. ~20–30% cheaper than rebuilding both `(verify)`.

### 14. Path decimation: don't tessellate finer than the pixel grid
A 160-point transfer curve on a ~184 px well is Nyquist-overkill. Decimate to ~1 vertex per
2–3 screen px (cull when horizontal delta `< minSpacingPx`); cuts vertex count ~160 → ~90–100 with
zero visual loss. Matters most for the 7 harmonics bars. Small CPU win, free quality.

### 15. Coalesce repaints (micro-optimization, profile first)
JUCE already batches repaints within one event-loop cycle, so flagging a `needsRepaint` dirty bit
in setters and calling a single `flushRepaints()` at the end of `timerCallback` is a refinement,
not a fix. The bigger question: does the meter truly need 30 Hz? If it looks good at 20 Hz,
`startTimerHz(20)` saves ~33% of timer overhead. (When you move to `VBlankAttachment` per §7, this
becomes vsync-bound anyway.)

---

## IV. DSP & state-model governance

### 16. DSP chain contract: Saturation → Convolution → RTNeural → DryWet (linear, mandatory)
```
in → saturation.process()      [16x/32x oversampled; defines latency]
   → convolution.process()     [cab IR; skip if !cabEngage]
   → neuralModel.process()     [learned character; skip if !neuralEngage]
   → dryWet.applyCrossfade()   [equal-power mix]
   → out
```
Every stage exposes `prepare(sr, block)` / `process(AudioBuffer)` / `reset()`, is RT-safe by
contract, and reports latency via `getLatencySamples()`. Use modern `juce::dsp::` types
(`juce::dsp::IIR::Filter`, `juce::dsp::Oversampling`, `juce::dsp::Convolution`) — NOT the legacy
`juce::IIRFilter` / `juce::Reverb` API. Both exist in JUCE 8; legacy is discouraged. Custom
modules (RTNeural wrapper) follow the same prepare/process/reset contract.

### 17. Oversampling: 16× realtime / 32× offline; document FIR order + stopband
Saturation defines total latency. Detect mode with `isNonRealtime()` in `prepareToPlay`:
16× realtime (FIR order ≈ 120, stopband ≈ 100 dB), 32× offline (FIR order ≈ 240, stopband
≈ 140 dB) — figures are starting points `(verify with an alias sweep through the 32× path)`.
Document the order + attenuation in code; never assume.

### 18. Equal-power dry/wet, not linear
`DryWet::applyCrossfade` uses the equal-power (tangent) law so perceived loudness is preserved as
`mix` sweeps. The crossfade preserves level (headroom contract: input nominal ±1.0 + transient
margin, saturation ceiling ≤ 1.0).

### 19. Report latency once; do NOT churn PDC on preset/IR swaps
Calling `setLatencySamples(N)` forces the host to recompute the whole session's Plugin Delay
Compensation graph; doing it on every preset change can stall transport / glitch automation.
v1 reported only the saturation stage (masked because IRs are ≤ 2048 taps zero-latency).

```cpp
const int total = saturation.getLatencySamples()
                + (cabEngage ? convolution.getLatencySamples() : 0);
if (total != getLatencySamples())          // ONLY if it actually changed
    setLatencySamples(total);
```
Construct convolution **zero-latency** (`juce::dsp::Convolution()`); with trimmed mono IRs all six
cabs stay 0, RTNeural is feed-forward (0), so total is fixed → compute once at `prepare`, and the
guarded call above effectively never fires in v1. Add a cab with latency > 0 later → the guard
handles it correctly. Never call `setLatencySamples()` inside `processBlock`.

### 20. `suspendProcessing(true/false)` around heavy, non-RT-safe critical sections
`suspendProcessing(true)` stops the host calling `processBlock` (returns silence) so the chain can
do heavy / blocking work safely. Current IR + neural swaps are already safe (JUCE convolution loads
on a background thread; RTNeural builds in an inactive slot then atomically swaps via `activeIdx`),
so it is optional today. **Bracket the call when:** a preset load touches multiple stages at once
(drive + cab + neural), or a future load path adds a blocking step (e.g. IR silence validation) on
the message thread. Call it from the message thread only, never inside `processBlock`.

### 21. `AudioParameterBool` (bypass) uses `get()` / `getToggleState()`, not slider floats
A bool param does not map to the 0–1 slider range. Read the truth with `bypassParam->get()`; write
with `setValueNotifyingHost(0.0f/1.0f)`. In the UI, drive it from `toggleButton.getToggleState()`.
Host sees a binary 0/1 automatable param. (We mirror it into `bypassState` atomic per §5.)

### 22. Decoupled-XML state is message-thread-only; data flows ONE way to the audio thread
The `WonKnobberState` struct (magic-header + XML, decoupled from the param list) is the preset /
session source of truth. The audio thread must NEVER read it directly.

```
XML (host blob) → WonKnobberState struct → AudioParameters → (audio thread via SmoothedValue/atomics)
```
`applyState()` is the single funnel: it reads the struct and writes the thread-safe
`AudioParameters`; the audio thread reads parameters, never the struct or XML. A/B slots are
`WonKnobberState` instances on the message thread, swapped in by writing parameters. Hosts usually
block the audio thread during `setStateInformation`, but there is no guarantee — the one-way flow
makes a half-updated read impossible. Never access XML / the state struct from `processBlock`.
Appending a parameter must not break a saved v1 session (schema `version` written AND read).

---

## V. Build, install & distribution hygiene (macOS)

### 23. CMake / JUCE 8 header generation order
JUCE 8 ships no prebuilt `JuceHeader.h`; `juce_generate_juce_header(target)` must come **after**
`target_link_libraries(target … juce::…)`. Out of order → an incomplete/missing header. Better:
in headers, include **individual modules** (`#include <juce_audio_processors/juce_audio_processors.h>`,
`<juce_graphics/juce_graphics.h>`, `<juce_dsp/juce_dsp.h>`) and reserve `JuceHeader.h` for `.cpp`
if at all — it isolates dependencies and survives module reshuffles.

### 24. The stale-instance trap — mandatory hygiene before every manual test
v1's #1 time-sink: build + install, but the DAW loads a cached / old-location binary, so changes
"vanish" and you debug the wrong binary. Caches are NOT invalidated on reinstall; changing
`PRODUCT_NAME` orphans the old bundle so both coexist.

1. Remove old bundles:
   - `~/Library/Audio/Plug-Ins/VST3/WON KNOBBER.vst3`
   - `~/Library/Audio/Plug-Ins/Components/WON KNOBBER.component` (if any legacy AUv2 lingers)
2. `killall -9 AudioComponentRegistrar` — else Logic/GarageBand load stale AU metadata.
3. Clear per-DAW caches (Ableton `Preferences.cfg` AU section; Logic restart;
   `~/Library/Caches/com.apple.sharedfilelist`; Reaper → re-scan VST3 folder).
4. Rebuild Release, reinstall, **cold-start** the DAW (not resume), load fresh, confirm a visible
   change in the UI.

Never `codesign --deep` (corrupts the bundle / nested frameworks → "unverified developer").
Never expect a recompile to be picked up without clearing old binaries first.

### 25. Build-stamp label — always know which binary you're running
Embed a build timestamp/type so you never profile or "fix" a stale binary.
```cpp
#if JUCE_DEBUG
    g.setColour(juce::Colour(0xff666666)); g.setFont(juce::Font(8.0f));
    g.drawText(__DATE__ " " __TIME__, 4, 4, 200, 10, juce::Justification::topLeft, false);
#endif
```
(Or a discreet `"WK v" WK_VERSION " · " __DATE__` bottom-right, visible at 100% zoom.)

### 26. Code signing + notarization (distribution, not local dev)
Ad-hoc signing is insufficient for distribution and M-series Gatekeeper. For release:
1. Sign with a **Developer ID Application** cert — `codesign --force --verify --verbose --sign
   "Developer ID Application" "…/WON KNOBBER.vst3"`. Never `--deep`.
2. `xcrun notarytool submit … --apple-id … --team-id … --password @keychain:notary-password`.
3. `xcrun stapler staple "…/WON KNOBBER.vst3"`.
4. Installer (`.dmg` / `.pkg`): sign then notarize the installer too.
AUv3 alternatively ships via the Mac App Store (different signing + sandbox). For LOCAL dev,
ad-hoc is fine: `codesign --force --sign - "…/WON KNOBBER.vst3"`. CI does sign/notarize on Release
only (Debug is dev-only).

### 27. macOS app icon for the AUv3 Standalone wrapper (JUCE 8.0.13+)
The AUv3 builds as an `.appex` inside a Standalone host `.app`; that `.app` needs an icon
(Finder/Launchpad). JUCE 8.0.13 prefers `.icon` bundles (Apple Icon Composer) or `.icns`; wire via
`ICON_BIG` in `juce_add_plugin`. Match the amber/dark identity (iron `#141517`, amber `#ffb74d`;
see DESIGN_SYSTEM §6).

---

## VI. Native UI input/metering patterns (universal, not WebView-specific)

### 28. Rotary knobs use RELATIVE (frame-delta) drag, not absolute cursor position
Absolute mapping snaps the knob when you click mid-range and spins wildly on large drags — the
industry rejected it 20+ years ago. Hook relative drag at the slider's mouse handler (the filmstrip
painter just selects a frame from the 0–1 value):
```cpp
void mouseDown(const juce::MouseEvent& e) override { dragStartY = (float) e.y; }
void mouseDrag(const juce::MouseEvent& e) override {
    const float dy = (dragStartY - (float) e.y) / (float) getHeight();
    value = juce::jlimit(0.0f, 1.0f, value + dy);     // ~1 range per height; tune sensitivity
    dragStartY = (float) e.y;                          // re-baseline each frame
    repaint();
}
```
Knob stays under the finger; smooth predictable acceleration; matches macOS-native + pro-DAW feel.

### 29. Ballistic VU motion: fast attack, slow decay, audio→atomic→UI decoupled
Pixel-for-pixel metering looks jittery. Real meters have slow ballistics. Audio thread writes a
**target** via atomic; UI thread (timer / `VBlankAttachment`) smooths it. (Our `IOMeter` already
does instant-rise + time-based linear decay per §8 — this is the canonical pattern for any future
scope, e.g. a Phase-3 spectrum analyzer.)

---

## VII. CI gates — executable, not prose (each FAILS the build)

macOS-only matrix (AUv3 + VST3). On every PR:
- **Zero-warning build** — Debug + Release, `-Werror`.
- **`pluginval` strictness 8–10** on the VST3; **`auval`** on the AUv3 → PASS.
- **RT-safety spot-check** — `clang-analyzer` / review on `processBlock` + DSP; flag any `new`,
  `malloc`, lock, or I/O.
- **Latency check** — reported samples == saturation + (cab IR if engaged); no mid-session churn.
- **Alias sweep** — through the 32× offline path; confirm stopband.
- **Visual regression** — `-DWK_BUILD_RENDER_HARNESS=ON`, render the **real editor** (not just the
  root view) at 1× and 2×, pixel-diff vs `tests/render/golden/` (front: diamond/mid, onyx/high,
  bypassed, voice-active; rear: engaged/about/licences). Fail on > ~0.1% diff. **Agents may NOT
  update goldens without human sign-off.**
- **Asset validation (pre-build, hard-fail)** — 1× plate == `kRefW×kRefH`, @2× == exactly 2×; every
  filmstrip == 256×(256·120) = 120 frames with a frame-0-vs-119 checksum guard against inversion;
  `static_assert` / single-source constant tying **baked transport-well count == coded function
  count**.
- **State round-trip** — load all 8 factory presets; save → reload → audio + UI match; automation
  re-links to the frozen `ParamID`s.
- **Release hygiene** — `strings` on the Release binary = 0 self-test markers.
- **Licences** — all third-party notices surfaced in-plugin (single-sourced from the rear modal).

### Profiling playbook (before optimizing, measure)
- **melatonin_perfetto** — `FetchContent` the module; `TRACE_COMPONENT("TransferCurve")` etc. in
  each paint; profile a **Release** build idling ~5 s in AudioPluginHost; open the trace in
  `ui.perfetto.dev`. Targets: editor paint total < 5 ms / 30 Hz tick; per-component < 1 ms
  (knob < 0.5 ms); **idle** `FaceplateView::paint` called 0–1×/sec (if it fires at 30 Hz idle,
  something is calling `repaint()` for free — find it).
- **Xcode Instruments → Core Animation** — Color Blended Layers: green = opaque (fast), red =
  transparent composite (slow), blue = offscreen. After §11 the faceplate should read green.
- **`JUCE_ENABLE_REPAINT_DEBUGGING=1`** (Debug only) — flashes repainted regions. Expected: only
  knob/meter/curve flash. Bad: whole `FaceplateView` flashes per tick (cascade). Worse: the
  faceplate PNG flashes (static layer re-rasterizing). Disable in Release (overhead).

---

## VIII. v1 post-mortem gotchas — these must NOT recur

1. **Floating controls** — code redrew button bodies offset from baked wells. → Plate bakes
   bodies; code draws overlays only, anchored to **measured well centres** (not mockup coords).
2. **Short plate export** — PNG ~12 px shorter than `kRefH`; bottom rim + ENGAGED clipped *in the
   file*. → Export the full canvas; assert `plate.height == kRefH == editor.height`.
3. **Bake/code well-count mismatch** — 5 baked transport domes, 4 coded functions → orphan well.
   → Lock the function list before baking; `static_assert` wells == functions.
4. **Anchor drift** — mockup anchors 20–40 px off the shipped plate. → Measure from the shipped PNG
   (connected-components / overlay-verified).
5. **Knob orientation churn** — false "verified min-first" claims. → Verify from the pixels
   (frame-0 vs frame-119 crop); bake min-first OR invert in code, never both.
6. **Editor-size / host caching** — window-size confusion across versions. → One final editor size;
   load a fresh instance on any breaking change.
7. **Verification blind spot** — harness rendered the view, not the editor. → Harness renders the
   real editor path (`createEditor()` + `setSize()`).
8. **Build/branch hygiene** — corrected assets lived only in a design sandbox; builds used stale
   files; concurrent agents squash-merged divergence. → **Single source of truth = the repo on
   disk.** Git worktrees for PR branches; explicit `git add <path>` (never `-A`). No sandbox→disk
   bridge.

---

## IX. Self-improving troubleshooting DB

Capture every failure + fix; promote solved issues into this file so they become preventive.

```markdown
### Issue: <short title>
Symptom: <what the error looks like>
Root cause: <why it happened>
Fix: <the solution>
Prevention: <which rule above to add/strengthen>
```

Seed entry:
```markdown
### Issue: Latency mismatch (reported ≠ expected PDC)
Symptom: pluginval warns "latency changed mid-session"; host PDC offset.
Root cause: only saturation latency reported; convolution IR latency omitted.
Fix: total = saturation + (cabEngage ? conv : 0); call setLatencySamples only if changed.
Prevention: rule 19.
```

---

## X. Per-PR summary checklist

- [ ] Builds with **zero warnings** (`-Werror`).
- [ ] `processBlock` RT-safe (no alloc / lock / I/O) — spot-checked.
- [ ] All UI params **smoothed**; no zipper noise.
- [ ] All assets (plates, filmstrips, IRs, models) load on the **message thread**.
- [ ] No mutable statics / singletons / thread_local; immutable lookup tables OK.
- [ ] Animated repaints on `VBlankAttachment`; meter decay is `dt`-based + clamped.
- [ ] Paths/gradients are members, rebuilt on change; static faceplate layer opaque/buffered.
- [ ] Modals + bypass wash are child components, not `paintOverChildren()` work.
- [ ] Overlays centred on **measured** well coords, not mockup estimates.
- [ ] Filmstrip knob 120 frames, min-first, inversion-free (verified from pixels).
- [ ] Latency reported once = saturation + cab IR (if engaged); no PDC churn.
- [ ] `pluginval` + `auval` pass; render harness < 0.1% diff vs golden.
- [ ] `ParamID` order frozen (v1→v2 automation compat); state round-trips.
- [ ] Licences single-sourced in the rear modal.
