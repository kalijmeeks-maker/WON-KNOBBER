# Design inbox (Claude Design → code team)

Claude Design runs in the **browser only** — it is not in the `wkteam` tmux loop.
This folder is the handoff dock between Design and Claude/Grok.

## Workflow

### One-time setup

```bash
wk design login    # Chromium opens — log in to Claude, open your Design project, press ENTER in Terminal
wk design check    # confirms the chat box is reachable
```

### Autonomous (Playwright types in the real Design chat)

1. `wk design new` → edit **`HANDOFF.md`** (`PROMPT:`, `TO:`, `DID:`, `HANDOFF:`).
2. `wk design auto` — reads **`PROMPT:`**, types into Claude Design, waits, saves screenshot to this folder.
3. Export/download assets from the browser, commit paths under `Resources/`.
4. Fill **`DID:`** / **`HANDOFF:`** in `HANDOFF.md`.
5. `wk design push` (or `wk design auto --push`) → `docs/relay.md` → `wkteam`.

Project URL: [Claude Design faceplate](https://claude.ai/design/p/1530ffb6-6fcb-48a7-994b-a8f17675fa90?file=exports%2Ffaceplate-pro-background-960x612.png) in `~/.config/wk-team/config.json`.

### Manual (no automation)

`wk design open` → work in Safari/Chrome → commit → `wk design push`.

## Files

| File | Purpose |
|------|---------|
| `HANDOFF.template.md` | Blank form — `wk design new` copies to `HANDOFF.md` |
| `HANDOFF.md` | Your current design→code note (gitignored if local-only) |

Optional: drop PNG/zips here before moving them to `Resources/` — keep paths in `DID:`.

## Relay addressing

| `TO:` in HANDOFF.md | Who implements |
|---------------------|----------------|
| `grok` | GUI, PNGs, anchors, `wk test render` |
| `claude` | Tokens in code, processor plumbing, CMake/tests |

Design owns aesthetics; Code/Grok implement and bring renders back for pixel-check.