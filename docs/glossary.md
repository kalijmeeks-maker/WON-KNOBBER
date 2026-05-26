# Glossary

- **VU ballistics** — the standardized rise/fall timing (~300 ms integration) that makes a
  VU meter respond like a physical needle rather than a peak meter.
- **Convolution** — applying an impulse response to a signal to impose another system's
  frequency/time response (e.g. a cabinet or hardware unit).
- **IR (Impulse Response)** — a recording of a system's response to an impulse; convolving
  with it reproduces that system's coloration.
- **NEBULA-style sampling** — capturing a hardware unit's behavior as sampled kernels/dynamic
  convolution rather than analytically modeling its circuit.
- **AUv3 sandboxing** — Audio Unit v3 runs the plugin in an app-extension sandbox with
  restricted filesystem/IPC access; assets must be bundled, not loaded from arbitrary paths.
- **Parameter smoothing** — ramping a parameter over time (`SmoothedValue`) so value changes
  don't cause clicks/zipper noise.
- **Denormal numbers** — tiny floating-point values near zero that are very slow to process;
  suppressed with `ScopedNoDenormals` to protect audio-thread performance.
- **Sample-accurate automation** — applying parameter changes at the exact sample offset the
  host scheduled them, rather than once per block.
