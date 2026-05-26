---
description: Configure and build the plugin with CMake (Debug) and report any errors.
allowed-tools: Bash(cmake:*), Read
---

Build WON-KNOBBER and report results.

1. Run: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug`
2. Run: `cmake --build build --config Debug`
3. If either step fails, show the first errors (file + line) and a one-line diagnosis each.
   Treat compiler warnings as failures (CI does).
4. On success, report the built artifact paths under `build/`.
