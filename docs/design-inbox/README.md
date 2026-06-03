# Design inbox (Claude Design → code team)

Claude Design runs in the **browser only** — it is not in the `wkteam` tmux loop.
This folder is the handoff dock between Design and Claude/Grok.

## Workflow

1. Work in **Claude Design** (browser): `wk design open`  
   (WON-KNOBBER project: faceplate background in [Claude Design](https://claude.ai/design/p/1530ffb6-6fcb-48a7-994b-a8f17675fa90?file=exports%2Ffaceplate-pro-background-960x612.png) — stored in `~/.config/wk-team/config.json` as `design_url`).
2. Save deliverables into the repo (`Resources/`, `docs/design-handoff-*.md`, etc.) and commit.
3. Fill in **`HANDOFF.md`** in this folder (copy from `HANDOFF.template.md`).
4. Run:

```bash
wk design push
```

That appends a `FROM design TO …` block to `docs/relay.md` and syncs worktrees.
Then `wkteam` (or an already-running relay) picks it up — **no copy-paste** between chats.

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