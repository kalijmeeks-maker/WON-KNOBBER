#!/usr/bin/env bash
set -uo pipefail
P='Read ./AGENTS.md (your lane + hard rules) and the newest docs/relay.md block addressed TO YOU. Execute its HANDOFF. Commit ONLY your lane files on your branch (explicit paths; never git add -A). Append your reply block to ./docs/relay.md. Commit docs/relay.md. Stop on NEEDS: or when your HANDOFF field ends with a lone line: HANDOFF: done. Do not ask the human to type in tmux — they steer only via docs/relay.md (FROM human TO …).'
MB="/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/docs/relay.md"
TUNER="/Users/kalimeeks/bin/relay-tuner.py"
STATE="/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/relay-state.json"
check_done(){ python3 "$TUNER" done "$MB" 2>/dev/null | grep -q '^True$' && { echo "✅ done"; exit 0; }; }
sync_out(){ cp -f "$1" "$MB" 2>/dev/null; cp -f "$1" "$2" 2>/dev/null; }
sync_mailbox(){
  cp -f "$MB" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude/docs/relay.md" 2>/dev/null
  cp -f "$MB" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok/docs/relay.md" 2>/dev/null
  [ -f "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/AGENTS.md" ] && cp -f "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/AGENTS.md" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude/AGENTS.md" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok/AGENTS.md" 2>/dev/null
}
stat_claude(){ git -C "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude" --no-pager diff --stat agent/grok...HEAD 2>/dev/null || git -C "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude" --no-pager diff --stat HEAD~3..HEAD 2>/dev/null; }
stat_grok(){ git -C "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok" --no-pager diff --stat agent/claude...HEAD 2>/dev/null || git -C "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok" --no-pager diff --stat HEAD~3..HEAD 2>/dev/null; }

plan_round(){
  local rnd=$1 grok_last=$2
  local cstat gstat plan
  cstat=$(stat_claude | tail -20)
  gstat=$(stat_grok | tail -20)
  plan=$(python3 "$TUNER" plan "$MB" "$STATE" "$rnd" "15" "$grok_last" "$cstat" "$gstat" 2>/dev/null) || plan='{"claude_max":24,"grok_max":6,"run_grok":false,"reason":"tuner fallback"}'
  echo "$plan"
}

mkdir -p "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude/docs" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok/docs"
sync_mailbox
GROK_RAN_LAST=0
for i in $(seq 1 15); do
  sync_mailbox
  PLAN=$(plan_round "$i" "$GROK_RAN_LAST")
  CM=$(echo "$PLAN" | python3 "$TUNER" field claude_max)
  GM=$(echo "$PLAN" | python3 "$TUNER" field grok_max)
  RC=$(echo "$PLAN" | python3 "$TUNER" field run_claude)
  RG=$(echo "$PLAN" | python3 "$TUNER" field run_grok)
  RS=$(echo "$PLAN" | python3 "$TUNER" field reason)
  echo "$PLAN" | python3 "$TUNER" field stop | grep -q True && { echo "tuner: stop"; exit 0; }
  check_done

  if [ "$RC" = "True" ]; then
    echo "═══ ROUND $i/15 ═══ CLAUDE max-turns=$CM │ $RS"
    if [ "$i" -eq 1 ]; then SES="--session-id e4b09006-2578-43ce-8ae1-ef51ac0d9267"; else SES="--resume e4b09006-2578-43ce-8ae1-ef51ac0d9267"; fi
    ( cd "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude"
      [ "$i" -gt 1 ] && { git merge --no-edit -q agent/grok 2>/dev/null || git merge --abort 2>/dev/null; }
      claude -p "$P" $SES         --permission-mode acceptEdits --max-turns "$CM"         --append-system-prompt-file "/Users/kalimeeks/agent-relay-prompt.txt"         --append-system-prompt-file "/Users/kalimeeks/agent-relay-prompt-claude.txt" --verbose ) || echo "[claude paused]"
    sync_out "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude/docs/relay.md" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok/docs/relay.md"; check_done
  else
    echo "═══ ROUND $i/15 ═══ CLAUDE skipped (mailbox→$(echo "$PLAN" | python3 "$TUNER" field last_to)) │ $RS"
  fi

  PLAN=$(plan_round "$i" "0")
  RG=$(echo "$PLAN" | python3 "$TUNER" field run_grok)
  GM=$(echo "$PLAN" | python3 "$TUNER" field grok_max)
  RS=$(echo "$PLAN" | python3 "$TUNER" field reason)

  if [ "$RG" = "True" ]; then
    echo "── GROK max-turns=$GM │ $RS"
    if [ "$i" -eq 1 ]; then GSES="-s 4dd2fa13-3621-4407-b1b6-d4923166fb08"; else GSES="-r 4dd2fa13-3621-4407-b1b6-d4923166fb08"; fi
    ( cd "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok"
      git merge --no-edit -q agent/claude 2>/dev/null || git merge --abort 2>/dev/null
      grok -p "$P" $GSES         --permission-mode acceptEdits --max-turns "$GM"         --rules "$(cat "/Users/kalimeeks/agent-relay-prompt.txt")"         --rules "$(cat "/Users/kalimeeks/agent-relay-prompt-grok.txt")" ) || echo "[grok paused]"
    sync_out "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/grok/docs/relay.md" "/Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/claude/docs/relay.md"; check_done
    GROK_RAN_LAST=1
  else
    echo "── GROK skipped (tuner) — $RS"
    GROK_RAN_LAST=0
  fi
done
echo "Finished 15 rounds. Tuning log: /Users/kalimeeks/Documents/GitHub/WON-KNOBBER/.agent-worktrees/relay-state.json"
