# Conventions

## Code Style

- C++17, 4-space indent, Allman braces, `ColumnLimit 120`, pointer-left.
- `#pragma once`; qualify `juce::`; no `using namespace juce;` in headers.
- `PascalCase` types, `camelCase` methods, lowercase `ParamIDs` string constants.
- Format with `clang-format` (config in repo root) before every commit.

## Commit Messages

Conventional Commits: `<type>(scope): description`. One concern per commit.
Types: feat, fix, docs, chore, build, ci, refactor, test, perf.

## Branch Naming

`feat/`, `fix/`, `chore/`, `docs/` + kebab-case slug.

## File Header Template

    /*
        <FileName> — <one-line responsibility>
        WON-KNOBBER · part of the <dsp|gui|core> layer
    */
    #pragma once
