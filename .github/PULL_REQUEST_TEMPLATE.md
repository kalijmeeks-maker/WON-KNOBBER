## Summary

One or two sentences on what this PR does.

## Changes

- 

## Tested In

DAWs / hosts (e.g. AudioPluginHost, Ableton Live 12, Reaper) and OS.

## Audio Impact

Signal-path / parameter / latency changes.

## GUI Impact

Component / asset / layout changes.

## Performance Impact

Audio-thread safety: confirm no allocations / locks / I/O added to `processBlock`.

## Checklist

- [ ] Builds on macOS and Windows (CI green)
- [ ] Zero compiler warnings
- [ ] `clang-format` applied
- [ ] No binaries / `build/` / `Builds/` / `JuceLibraryCode/` committed
- [ ] Conventional Commit messages
- [ ] Docs updated if needed
