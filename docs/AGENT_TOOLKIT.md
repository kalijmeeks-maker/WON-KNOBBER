# Agent toolkit (WON-KNOBBER)

Team-wide CLI lives on the Mac host: **`wk`** — see `~/.config/wk-team/PLAYBOOK.md` or run:

```bash
wk playbook
wk help
```

Quick verify after a GUI PR:

```bash
wk test render    # if WonKnobberRender is built
wk test auval
wk pin flip && wk drive flip bypass
```

Relay: `wk agent .` from this repo (requires `AGENTS.md` + `docs/relay.md` at repo root or parent protocol).

**Start your day:** `wk day` (or `wk day /path/to/any-relay-repo`) — health, relay tail, worktrees, tmux status, which folder to open in Claude Code.

### Agents: you are not in a chat pane

`wkteam` opens tmux for **monitoring**, not conversation. Your turn is **`docs/relay.md` +
git** only. See `AGENTS.md` § Human / tmux. Shared rules also ship in `~/agent-relay-prompt.txt`
(appended every relay invocation).

### Human: where to interact

| Goal | Where |
|------|--------|
| Watch the team | tmux RELAY + MONITOR (detach `Ctrl+B` `D`) |
| Steer agents | Edit `docs/relay.md` (`FROM human TO …`) |
| Talk to one agent | Cursor, or `claude` / `grok` in a normal terminal (pause loop first) |
| Stop | `Ctrl+C` in RELAY LOOP, or agent writes `HANDOFF: done` on its own line |