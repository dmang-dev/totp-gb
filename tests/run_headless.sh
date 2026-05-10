#!/usr/bin/env bash
#
# run_headless.sh — automated end-to-end verifier for totp-gb-test.gb.
#
# Loads the test ROM in mGBA-SDL (under xvfb), runs verify_headless.lua
# which polls the live screen and writes PASS/FAIL into result.txt, then
# asserts and exits 0 / 1.
#
# Requires: mgba-sdl (>= 0.10), xvfb-run.
# Tested target: GitHub Actions ubuntu-latest.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
ROM="${ROM:-$ROOT/artifacts/totp-gb-test.gb}"
SCRIPT="$HERE/verify_headless.lua"
RESULT="$HERE/result.txt"
TIMEOUT_SEC="${TIMEOUT_SEC:-15}"

if [[ ! -f "$ROM" ]]; then
    echo "ERROR: ROM not found at $ROM (run ./build.bat test first)" >&2
    exit 2
fi

rm -f "$RESULT"

echo "[run_headless] launching mGBA against $(basename "$ROM")..."
xvfb-run -a mgba \
    -l 31 \
    --script "$SCRIPT" \
    "$ROM" >/tmp/mgba.log 2>&1 &
MGBA_PID=$!

deadline=$((SECONDS + TIMEOUT_SEC))
while (( SECONDS < deadline )); do
    [[ -s "$RESULT" ]] && break
    sleep 0.25
done

kill "$MGBA_PID" 2>/dev/null || true
wait "$MGBA_PID" 2>/dev/null || true

if [[ ! -s "$RESULT" ]]; then
    echo "[run_headless] TIMEOUT after ${TIMEOUT_SEC}s" >&2
    tail -n 40 /tmp/mgba.log >&2 || true
    exit 1
fi

verdict="$(head -n1 "$RESULT")"
echo "[run_headless] $verdict"
case "$verdict" in
    PASS*) exit 0 ;;
    *)     tail -n 40 /tmp/mgba.log >&2 || true; exit 1 ;;
esac
