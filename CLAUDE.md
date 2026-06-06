# Project: WON-KNOBBER — JUCE Audio Plugin

## Targets (HARD CONSTRAINT)
- **AUv3 + VST3 ONLY.** macOS only. (NOT AUv2 / `.component` — per owner directive 2026-06-06.)
- Do NOT add AAX, iLok, Windows, or Linux targets. No standalone unless asked.
- C++20, JUCE 8.x, CMake (Pamplejuce-style), Catch2 for tests.

## Architecture (carry-forward spec — do not deviate without asking)
- **Faceplate + seated overlays:** Photoreal PRO chassis as background image.
  Controls are live overlays drawn into the plate's recesses/wells —
  "seated, not painted-on." Never paint controls as floating widgets.
- **Reference-coordinate layout:** ONE `kRefW x kRefH` reference. Every control
  placed via `place(x, y, w, h)` that scales from a single plate-truth JSON.
  All anchors come from that JSON — no hardcoded magic numbers in resized().
- **DSP chain (RT-safe processBlock):**
  Airwindows saturation (oversampled) → 6-IR convolution → RTNeural →
  equal-power dry/wet.
- **Oversampling:** 32x offline / 16x realtime as the MAX. Use
  juce::dsp::Oversampling; switch factor on isNonRealtime().
- **Controls:** one meaningful macro (drive) + gem filmstrip knob
  (120 frames, min-first, inversion-free LookAndFeel, 7 stone variants).
  Identity = amber/dark.
- **State:** raw addParameter + custom magic-header XML blob DECOUPLED from
  the param list. Adding params must NOT break saved sessions.
- **Host behavior:** bypass param + PDC, automation begin/endChangeGesture,
  modal scrim + Esc to dismiss. Cab/neural live on a REAR panel; front stays clean.

## Verification (give yourself a check you can run — ALWAYS run these)
- `ctest` (Catch2 unit tests) must pass.
- `pluginval --strictness-level 10` must pass on both AU and VST3.
- `auval -v aufx <subtype> <manu>` must pass for the AU.
- Headless render harness: createComponentSnapshot → PNG on the REAL editor
  (not a detached view), diffed against golden PNG. Must match within tolerance.
- processBlock must be RT-safe: NO allocation, locks, logging, or file I/O.

## Workflow rules
- Explore → Plan → Code → Commit. Use plan mode; let me approve the plan first.
- Run the verification loop after every meaningful change. Don't trust, verify.
- When context gets noisy, /clear and re-read this file + critical-patterns.md.
- Required reading before touching DSP or GUI: ./docs/juce-critical-patterns.md

## Companion docs (canonical set — root is the source of truth)
- `docs/juce-critical-patterns.md` — Required Reading: 19-rule quick-ref + code-level detail.
- `docs/parallel-worktree-workflow.md` — 4-stream parallel build (dsp / gui / state / build).
- `v2/DESIGN_SYSTEM.md` — visual + interaction rulebook (faceplate-bake doctrine, gem filmstrip,
  §9 GUI tooling stack: melatonin_blur/inspector, native 2× assets, vector overlays, JUCE 9 wins,
  Visage-watch — Direct2D/WebView marked N/A for our macOS-only native scope).
- `v2/V2_BRIEF.md` — product + engineering spec, v1 post-mortem, executable gates, owner decisions.
- `.claude/settings.json` + `scripts/verify.sh` — deterministic gates (clang-format on edit;
  ctest + pluginval + auval + rt_safety/render checks at Stop). `verify.sh` is a forward template:
  the `|| true` guards drop once the target builds; fill the `auval` subtype/manu.

## Live docs (MCP) — owner step (agents don't auto-load MCP)
Wire current JUCE/library docs so the agent uses real APIs, not deprecated ones. Create `.mcp.json`:
`{"mcpServers":{"context7":{"command":"npx","args":["-y","@upstash/context7-mcp"]}}}` — and
optionally add the JUCE-docs MCP (github.com/josmithiii/mcp-servers-jos) once its run command is
confirmed.

## Multi-agent collaboration (Claude Design + GrokBuild + Claude Code — seamless, no drag-drop)
All agents (Claude Design for visuals/interaction, GrokBuild for UI/assets/docs, Claude Code for DSP/state/processor) MUST bootstrap every session the same way to stay on one page:
1. Read root `CLAUDE.md` (this file) in full.
2. Read `docs/juce-critical-patterns.md` (the 19 rules + WON-KNOBBER code examples).
3. Read the **newest block** at the bottom of `docs/relay.md` (and the one before it for context).
4. Read relevant files from `repo-handoff/docs/` for any open design handoff (e.g. PLATE_BOTTOM_CLIP, GEM_IR_LANE_SYNC, GROK_WORKORDER, CC_UPDATE).
5. Communicate **only** by appending a single newest-last block to `docs/relay.md` in the exact format (see AGENTS.md). Never paste code, diffs, or long excerpts between chats — use the relay or handoff docs.
6. When you need codebase intelligence, use the Context7 MCP (see .mcp.json). Do not ask the human to paste files.

GrokBuild (this session) and Claude Design must both be pointed at the **same project root**:
`/Users/kalimeeks/Documents/GitHub/WON-KNOBBER`

In Claude Desktop/Projects: Attach the entire folder above as a Project (or Knowledge source). This gives direct file access + the MCP for search. No manual copy-paste required after the initial attach.

GrokBuild here already has native filesystem access to the root; it will always re-read the four items above at the start of any complex task.

This eliminates drag-and-drop. The relay.md + repo-handoff/ + canonical CLAUDE.md are the single source of truth.
