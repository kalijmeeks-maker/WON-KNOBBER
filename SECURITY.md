# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in WON-KNOBBER, please **do not** open a public issue.

Instead, report it privately via GitHub's [private vulnerability reporting](https://github.com/kalijmeeks-maker/WON-KNOBBER/security/advisories/new), or contact the maintainer directly:

- **Maintainer:** [@kalijmeeks-maker](https://github.com/kalijmeeks-maker)

You can expect an initial response within a reasonable timeframe. Please include:

- A description of the issue and its potential impact
- Steps to reproduce, or a minimal proof of concept
- The affected version / commit hash
- Your environment (DAW, OS, plugin format: VST3 / AU / AUv3)

## Supported Versions

This project is in active development and has not yet shipped a stable release. Security fixes are applied to the latest commit on `main`.

## Scope

In scope:

- Memory safety issues in the DSP / plugin core
- Crashes that can be triggered by malformed input (audio buffers, MIDI, plugin state, preset files)
- Issues that could lead to arbitrary code execution inside a host DAW
- Insecure handling of preset / model files

Out of scope:

- Bugs that only affect audio quality, UI rendering, or correctness without security implications (please open a regular issue)
- Issues in third-party dependencies (please report upstream)
