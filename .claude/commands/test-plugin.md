---
description: Locate the freshly built plugin and load it in JUCE AudioPluginHost.
allowed-tools: Bash, Read
---

Smoke-test the built plugin.

1. Locate the built VST3 (and AU on macOS) under `build/` (e.g. `build/**/*.vst3`).
2. Launch JUCE's `AudioPluginHost` with that plugin if available on PATH; otherwise print the
   exact path and tell the user to drag it into AudioPluginHost.
3. Manual checks to report back: knob turns the `drive` parameter, no clicks/zipper noise
   (smoothing works), VU meter responds to signal, bypass LED toggles.
