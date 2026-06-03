# AGENTS.md — Claude ↔ Grok autonomous relay (WON-KNOBBER)

Two agents collaborate via **`docs/relay.md`**. Human starts the loop (`wkteam`); then **no copy-paste**.

## Human / tmux (read this — not a chat UI)

| tmux pane | Agent use |
|-----------|-----------|
| RELAY LOOP | Loop only — human does not type here |
| MONITOR | Read-only status |
| CLAUDE / GROK | Spare shells; **not** your turn channel |

You run via **`claude -p` / `grok -p`** each round. Do not ask the human to type in tmux or
paste context between agents. Human steers by appending **`FROM human TO …`** blocks in
`docs/relay.md`, or via **`NEEDS:`** (non-`none`). Merge to `main` is always human.

**Token policy (dynamic):** `relay-tuner.py` adjusts each round from `docs/relay.md` +
git diff stats. Claude = default heavy; Grok runs only when HANDOFF targets GUI / small
wiring tasks — skipped after noop Grok turns or late-round Claude-only taper.
State: `.agent-worktrees/relay-state.json` (watch in tmux MONITOR pane).

## Hard rules

- Work only in your worktree branch: **`agent/claude`** or **`agent/grok`**.
- Stage by **explicit path** only. Never `git add -A` / `git add .`.
- Never force-push, merge to `main`, delete branches, or rewrite history → `NEEDS:` in relay and stop.
- Ableton GUI: `wk playbook` — never `activate Live` while driving.

## Relay format (append ONE block per turn)

```
### <YYYY-MM-DD HH:MM> FROM <claude|grok> TO <grok|claude>
DID: <files changed>
HANDOFF: <exact next step for the other agent>
NEEDS: none
```

- **START:** read newest block **to you**; do its `HANDOFF`.
- **END:** commit your code; append your block; commit `docs/relay.md`.
- **STOP:** `HANDOFF: done`

## Lanes

### CLAUDE — processor / DSP / preset data (avoid `Source/gui/*`)

- `Source/PluginProcessor.{h,cpp}`, `Source/dsp/*`, preset/state schema
- `Source/Presets/PresetManager.{h,cpp}` if adding v1.1 disk presets
- Tests, CMake, `Resources/` factory XML

### GROK — GUI (avoid `PluginProcessor.*` and heavy DSP)

- `Source/gui/*`, `Resources/` PNGs, `docs/` design handoffs
- Wire UI to processor via editor callbacks only
- GUI verify: `wk test render` then `wk pin flip` / `wk drive` (not full Ableton every turn)

## Shared seam (propose in relay before changing)

```cpp
// PresetManager.h — example; confirm in relay before edit
juce::StringArray getFactoryNames();
juce::StringArray getUserNames();
void loadPreset(const juce::String& name);
void saveUserPreset(const juce::String& name);
void deleteUserPreset(const juce::String& name);
bool isModified() const;
std::function<void()> onPresetChanged;
```

## Git worktrees (do not confuse paths)

Same repo (`kalijmeeks-maker/WON-KNOBBER`). `git worktree list` on the Mac:

| Path | Branch | Relay? |
|------|--------|--------|
| `~/Documents/GitHub/WON-KNOBBER` | `main` | Mailbox source; human merges here |
| `…/WON-KNOBBER/.agent-worktrees/claude` | `agent/claude` | **Yes** — Claude autopilot |
| `…/WON-KNOBBER/.agent-worktrees/grok` | `agent/grok` | **Yes** — Grok autopilot |
| `~/Documents/GitHub/wk-rear-ui` | `feat/rear-panel-ui` | **No** — manual/Cursor; merge into `agent/grok` when ready |
| `~/Documents/GitHub/wk-grok-assets` | `feat/grok-asset-sync` | **No** |

Edits outside `.agent-worktrees/{claude,grok}` are invisible to the relay until merged into the
active agent branch. **Newest `TO <agent>`** in `docs/relay.md` sets who runs next (`relay-tuner`
skips Claude when `TO grok`, skips Grok when `TO claude`). End the run with HANDOFF field ending
in a lone line `HANDOFF: done` — never embed that phrase in prose (stops the loop early).

## Toolkit

`wk playbook` · `wk agent .` · repo `docs/AGENT_TOOLKIT.md`