#!/usr/bin/env bash
# Fast red/green check with an existing instrumented DOL and the reused SD profile.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
dol="${1:?usage: mario_kart_check.sh <instrumented.dol> [VIs] [diag options...]}"
vis="${2:-900}"
shift
if [ "$#" -gt 0 ]; then shift; fi
[[ "$vis" =~ ^[0-9]+$ && "$vis" -ge 600 ]] || { echo 'Use at least 600 VIs.' >&2; exit 2; }
mkdir -p .dev/runs
log="$(mktemp "$PWD/.dev/runs/mk64-check-XXXXXX")"
status=0
WII64_DOLPHIN_DSP_HLE=False bash .dev/dolphin_test.sh "$dol" "$((vis / 30 + 30))" \
    dynacore=dynarec audio_quality=accurate audio_output=dsp audio_mixer=accurate \
    audio_latency=stable audio_sync=native "$@" \
    "chain=$vis,input=neutral sd:/wii64/roms/Mario Kart 64.v64" > "$log" 2>&1 || status=$?
run="$(sed -n 's/^Dolphin chain results: //p' "$log" | tail -n 1)"
echo "Launch log: $log"
[[ -n "$run" ]] || { tail -n 20 "$log"; exit 1; }
echo "Run: $run"
python3 scripts/mario_kart_check.py "$run" || status=1
exit "$status"
