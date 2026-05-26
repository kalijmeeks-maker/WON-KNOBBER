# Contributing to WON-KNOBBER

Thanks for your interest in WON-KNOBBER. This project is primarily developed by [@kalijmeeks-maker](https://github.com/kalijmeeks-maker); contributions are welcome but please read this document first.

## License & contribution terms

WON-KNOBBER is distributed under the [PolyForm Noncommercial 1.0.0](LICENSE) license. By submitting a pull request, you agree that your contribution is licensed under the same terms and that the maintainer may relicense the project (including your contribution) in the future, including for commercial release.

If you cannot agree to those terms, please do not submit a pull request.

## JUCE licensing notice

WON-KNOBBER is built on [JUCE](https://juce.com/). JUCE is dual-licensed (GPL or commercial). If you build or distribute WON-KNOBBER yourself, **you are responsible for complying with JUCE's license terms**, including obtaining a commercial / Indie JUCE license if you ship a closed-source build.

## Workflow

1. Open an issue first for non-trivial changes so we can agree on scope.
2. Fork the repo and create a feature branch off `main`:
   - `feat/...` for features
   - `fix/...` for bug fixes
   - `chore/...` / `ci/...` / `docs/...` for maintenance
3. Keep PRs focused and reasonably small.
4. Make sure CI passes (macOS + Windows builds).
5. Open a PR against `main`. The branch is protected: no force pushes, no deletions, linear history, conversation resolution required before merge.

## Code style

- C++20, modern JUCE idioms.
- Prefer `juce::` types at the plugin boundary; pure DSP code can be JUCE-free where reasonable.
- 4-space indentation, no tabs in source files.
- Run `clang-format` if a `.clang-format` is present.

## Commit messages

Use imperative mood, scoped where useful:

```
feat(dsp): add convolution oversampling
fix(ui): correct knob hit-region on retina
chore(ci): bump actions/checkout to v4
```

## Reporting bugs

Open an issue with:

- DAW + version (Logic, Ableton, Reaper, Cubase, ...)
- OS + version
- Plugin format (VST3 / AU / AUv3)
- Sample rate and buffer size
- Steps to reproduce and expected vs. actual behavior

For security issues, see [SECURITY.md](SECURITY.md) — please do **not** open a public issue.
