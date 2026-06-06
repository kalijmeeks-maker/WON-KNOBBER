# Parallel worktree workflow

```bash
# Four streams, four worktrees, four Claude Code sessions in parallel.
git worktree add ../plugin-dsp    feature/dsp     # oversampling, Airwindows, conv, RTNeural
git worktree add ../plugin-gui    feature/gui     # faceplate, overlays, gem knob, LnF
git worktree add ../plugin-state  feature/state   # magic-header XML, param decoupling
git worktree add ../plugin-build  feature/build   # CMake, pluginval/auval, hooks, signing
```

Per stream: open Claude Code in that worktree, run plan mode (approve the plan before any code), let it code against the stream's slice of `CLAUDE.md`, then `/code-review` with fresh context, then merge to main only after `verify.sh` passes green. The DSP stream owns the 32x/16x oversampling spec; the GUI stream owns the seated-overlay + filmstrip + amber/dark identity; state owns the decoupled XML; build owns the AU+VST3 gates.
