#!/usr/bin/env bash
# Reuse an instrumented DOL and SD profile; no extra ROM copies or frame dumps.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
dol="${1:?usage: zelda_check.sh <instrumented.dol> [boot|menus|new_game] [diag options...]}"
mode="${2:-boot}"
shift
if [ "$#" -gt 0 ]; then shift; fi
case "$mode" in boot|menus|new_game) ;; *) echo 'Use boot, menus or new_game.' >&2; exit 2 ;; esac
args=()
while IFS= read -r line; do
    [[ -z "$line" || "$line" == \#* ]] || args+=("$line")
done < "scripts/chains/zelda_$mode.txt"
mkdir -p .dev/runs
log="$(mktemp "$PWD/.dev/runs/zelda-check-XXXXXX")"
status=0
# Two 32 MiB ROM loads can take a minute each. Guest hangs have their own watchdog.
WII64_DOLPHIN_DSP_HLE=False bash .dev/dolphin_test.sh "$dol" 600 \
    dynacore=dynarec audio_quality=accurate audio_output=dsp audio_mixer=accurate \
    audio_latency=stable audio_sync=native "$@" "${args[@]}" > "$log" 2>&1 || status=$?
run="$(sed -n 's/^Dolphin chain results: //p' "$log" | tail -n 1)"
echo "Launch log: $log"
[[ -n "$run" ]] || { tail -n 20 "$log"; exit 1; }
echo "Run: $run"
python3 scripts/zelda_check.py "$run" --both || status=1
exit "$status"
