---
description: Scaffold a real-time-safe DSP module (header + cpp) under Source/dsp/.
argument-hint: <ModuleName>
allowed-tools: Write, Read
---

Create a new DSP module named **$1** following project DSP conventions.

Generate `Source/dsp/$1.h` and `Source/dsp/$1.cpp`:

- Class `$1` in the project's dsp layer, with the standard contract:
  - `void prepare(double sampleRate, int blockSize)`
  - `void process(juce::AudioBuffer<float>& buffer)`
  - `void reset()`
- Store `sampleRate` and `blockSize`; no allocation/locks/I/O in `process`.
- `process` body wrapped logic must stay real-time safe; smooth any parameters with
  `juce::SmoothedValue<float>`.
- Header uses `#pragma once`, qualifies `juce::`, includes the file header template.
- Leave the algorithm body as a `// TODO: implement` stub — do NOT write DSP math.
