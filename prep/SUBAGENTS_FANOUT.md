# Sub-agents fanned out for handoff preparation (Grok session)

Date: Wed Jun  3 02:31:58 PDT 2026
Worktree: /Users/kalimeeks/Documents/GitHub/wk-grok-assets
Branch: feat/grok-asset-sync
HEAD: dd46d89

## Active background sub-agents (use get_command_or_subagent_output with their ids to retrieve reports when ready)

1. explore (ID: 019e8cd2-8940-7030-a356-b224fe8d0aa3)
   - Focus: Full audit of every cabIr / neuralModel ID string usage.
   - Will produce: PREP_ID_AUDIT.md (table of occurrences, mismatches vs voice-map + manifest, edit sites, canonical proposal).
   - Capability: read-only.

2. plan (ID: 019e8cd2-a93a-7900-a408-04cf7f748a99)
   - Focus: Actionable integration plan for when handoff lands (final IDs, updated map, rear anchors, etc.).
   - Will produce: PREP_INTEGRATION_PLAN.md (prioritized checklist, code sites, risks like BinaryData symbol stability, test strategy for clicks on preset cycle, latency, what to ask on handoff).
   - Capability: read-only.

3. won-qa (ID: 019e8cd2-c880-76d0-a132-9976e818d2d6)
   - Focus: Adversarial review of the current Convolution + NeuralModel + processor wiring + self-tests vs the rt_safe_swap + real-time rules in the integration_spec and spikes.
   - Will produce: PREP_QA_REPORT.md (per-rule pass/fail with evidence + line numbers, bugs found, additional tests needed, readiness verdict).
   - Capability: read-only.

4. general-purpose (ID: 019e8cd2-eb15-73e3-b4f1-4e6ed294c262)
   - Focus: Rear/flip UI readiness using the just-ingested flip-spec.html + rear-panel-anchors.json.
   - Will produce: PREP_REAR_UI_READINESS.md (summary of spec, current GUI gaps in FaceplateView/Editor, component suggestions, anchors-to-bounds mapping, build notes for rear bg/fonts, prep for CC's rear PR#53).
   - Capability: read-only.

## How to resume when handoff lands
- Call get_command_or_subagent_output for each of the 4 ids (with block=true if needed).
- cd to this worktree.
- cat prep/*.md
- The reports will contain the exact diffs, ID maps, checklists, and evidence needed to execute the next task (e.g. update IDs in 5 places + sync all 8 XMLs + extend tests + verify) without re-exploring from scratch.
- All in isolation on this branch; explicit git add paths only when making changes.
- After changes, new commit on this branch, then human can integrate.

## Context for sub-agents (they have this + instructions to read the files)
- Assets just refreshed with generators (locked ir_*.wav + model_*.json).
- Docs just ingested: full spec, manifest, spikes, flip-spec, anchors, updated pointer.
- Code has cab/neural schema + wiring + self-tests already (but ID strings and per-voice values in XMLs may need sync to the authoritative voice-cab-neural-map.md table).
- RTNeural + BinaryData already in CMake.
- Next handoff expected to resolve any ID canonicals (short vs TAPE-1971 etc), deliver final rear anchors or flip details, or say "go integrate the assets end-to-end + update presets".

Sub-agents are autonomous; they will use their own tool calls (read, grep, etc.) to explore and write their reports here.
