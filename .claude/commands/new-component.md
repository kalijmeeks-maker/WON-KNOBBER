---
description: Scaffold a juce::Component (header + cpp) under Source/gui/ with LookAndFeel hooks.
argument-hint: <ComponentName>
allowed-tools: Write, Read
---

Create a new GUI component named **$1** following project GUI conventions.

Generate `Source/gui/$1.h` and `Source/gui/$1.cpp`:

- Class `$1 : public juce::Component`.
- Override `void paint(juce::Graphics&)` and `void resized()`.
- Route all custom drawing through the active `juce::LookAndFeel` (do not hard-code paint
  logic that belongs in `KnobLookAndFeel`); add a `setLookAndFeel` hook if it owns a control.
- Dark-mode color tokens from `docs/gui.md`.
- Header uses `#pragma once`, qualifies `juce::`, includes the file header template.
- Leave drawing as a `// TODO` stub.
