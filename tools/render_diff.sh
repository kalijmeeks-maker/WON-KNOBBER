#!/usr/bin/env bash
#
# render_diff.sh — headless visual-regression check for the WON-KNOBBER GUI.
#
# Renders the current GUI offscreen via WonKnobberRender, then diffs each side (front/rear)
# against its LOCKED baselines in tools/baselines/<side>/. Per-side + independent, fuzz-tolerant,
# emits diff masks, CI-friendly exit code.
#
# Baselines are LOCKED per side only after Design's sign-off: drop that side's signed-off renders
# into tools/baselines/<side>/. The dirs are empty until then — an empty side simply passes (skips),
# so this is safe to wire up now and the first real lock is just a file copy.
#
# Usage: tools/render_diff.sh [--build]
#   --build   (re)build the WonKnobberRender target first (may fetch JUCE if not cached).
#
# Exit: 0 = every present baseline passed (or none locked yet); nonzero = a regression or error.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
HARNESS="$BUILD_DIR/WonKnobberRender_artefacts/Release/WonKnobberRender"
BASE_DIR="$ROOT/tools/baselines"
DIFF_DIR="$ROOT/tools/diffs"
FRESH_DIR="$DIFF_DIR/_fresh"
THRESHOLD=50     # AE pixels (after fuzz) above which an image is a regression
FUZZ="1%"        # swallow anti-aliasing / sub-pixel noise, not real diffs

mkdir -p "$BASE_DIR/front" "$BASE_DIR/rear" "$DIFF_DIR"

if [[ "${1:-}" == "--build" ]]; then
    echo "Building WonKnobberRender..."
    cmake -B "$BUILD_DIR" -S "$ROOT" -DCMAKE_BUILD_TYPE=Release -DWK_BUILD_RENDER_HARNESS=ON >/dev/null \
        || { echo "configure failed"; exit 2; }
    cmake --build "$BUILD_DIR" --config Release --target WonKnobberRender -j >/dev/null \
        || { echo "build failed"; exit 2; }
fi

if [[ ! -x "$HARNESS" ]]; then
    echo "harness not built: $HARNESS"
    echo "  build it:  cmake -B build -DWK_BUILD_RENDER_HARNESS=ON && cmake --build build --target WonKnobberRender"
    echo "  or re-run: tools/render_diff.sh --build"
    exit 2
fi

# Render the current GUI fresh.
rm -rf "$FRESH_DIR"; mkdir -p "$FRESH_DIR"
"$HARNESS" "$FRESH_DIR" >/dev/null || { echo "render failed"; exit 2; }

shopt -s nullglob
overall=0
for side in front rear; do
    files=("$BASE_DIR/$side"/*.png)
    if (( ${#files[@]} == 0 )); then
        echo "[$side] no baselines locked yet — skipping"
        continue
    fi
    echo "[$side] ${#files[@]} baseline(s)  (fuzz $FUZZ, fail if AE > $THRESHOLD px):"
    side_fail=0
    for base in "${files[@]}"; do
        name="$(basename "$base")"
        fresh="$FRESH_DIR/$name"
        if [[ ! -f "$fresh" ]]; then
            echo "  ! $name — no matching fresh render (skipped)"
            continue
        fi
        # AE count goes to stderr; capture it. Diff mask written to tools/diffs/<name>.
        ae="$(magick compare -metric AE -fuzz "$FUZZ" "$base" "$fresh" "$DIFF_DIR/$name" 2>&1 | grep -oE '^[0-9]+' | head -1)"
        ae="${ae:-999999}"
        if (( ae > THRESHOLD )); then
            echo "  ✗ $name  AE=$ae   (diff mask: tools/diffs/$name)"
            side_fail=1
        else
            echo "  ✓ $name  AE=$ae"
        fi
    done
    (( side_fail )) && overall=1
done
shopt -u nullglob

echo "RESULT: $([[ $overall -eq 0 ]] && echo PASS || echo FAIL)"
exit $overall
