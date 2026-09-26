#!/usr/bin/env bash
# Independent, compact Dolphin captures; each game gets a fresh profile.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh

target="${1:-glN64_wii}"
case "$target" in glN64_wii|Rice_wii) ;; *) echo "Use glN64_wii or Rice_wii." >&2; exit 2 ;; esac
filter="${WII64_TRIAGE_FILTER:-all}"
case "$filter" in all|mario-kart|mario-party) ;; *) echo "WII64_TRIAGE_FILTER must be all, mario-kart, or mario-party." >&2; exit 2 ;; esac
command -v python3 >/dev/null
mkdir -p .dev/runs

# Produce the stats and frame snapshots expected by chain_table.py.
case "${WII64_SKIP_BUILD:-0}:${target}" in
	1:glN64_wii) dol=wii64-glN64.dol ;;
	1:Rice_wii) dol=wii64-Rice.dol ;;
	*)
		build_log=".dev/runs/mario-title-build-$(date +%Y%m%d-%H%M%S).log"
		if ! .dev/build_profiling.sh "$target" >"$build_log" 2>&1; then
			tail -40 "$build_log" >&2
			exit 1
		fi
		echo "Profiling build complete; build log: $build_log"
		case "$target" in glN64_wii) dol=wii64-glN64.dol ;; Rice_wii) dol=wii64-Rice.dol ;; esac
		;;
esac
if [ ! -f "$dol" ]; then
	echo "Missing $dol; remove WII64_SKIP_BUILD=1 to build it." >&2
	exit 1
fi

run_case() {
	local label="$1" vis="$2" input="$3" rom="$4" seconds="$5"
	if [ "$filter" != all ] && [[ "$label" != *"$filter"* ]]; then return; fi
	local out profile input_arg line
	out="$(mktemp -d ".dev/runs/mario-${label}-XXXXXX")"
	profile="$PWD/$out/profile"
	mkdir -p "$profile/Config"
	cp .dev/dolphin_profile/Config/*.ini "$profile/Config/"
	input_arg=""
	[ -z "$input" ] || input_arg=",input=$input"
	line="chain=$vis$input_arg sd:/wii64/roms/$rom"
	printf '%s\n' "$line" > "$out/diag.cfg"
	echo "=== $label ($vis VI) ==="
	# First boot seeds WiiSD.raw from the staged sync folder; the diagnostic
	# boot then uses raw-image mode so a guest hang cannot hide perf.log.
	WII64_DOLPHIN_PROFILE="$profile" .dev/dolphin_test.sh "$dol" 15 "$line" result_host=127.0.0.1
	WII64_DOLPHIN_PROFILE="$profile" WII64_DOLPHIN_FOLDER_SYNC=False .dev/dolphin_test.sh "$dol" "$seconds"
	local wii64="$profile/Load/WiiSDSync/wii64"
	for name in "$wii64/perf.log" "$wii64"/xfb_*.bin "$wii64"/padtrace_*.csv; do
		[ ! -f "$name" ] || cp "$name" "$out/"
	done
	if [ -f "$out/perf.log" ]; then
		python3 scripts/chain_table.py "$out" | tee "$out/summary.txt"
	else
		echo "No perf.log returned; see $profile/Logs/dolphin.log"
	fi
	echo "Run files: $out"
}

# Long enough to expose the known MK64 stall; no-input and replay are separate
# processes so the first watchdog dialog cannot prevent later cases from running.
run_case mario-kart-no-input 3600 "" "Mario Kart 64.v64" 45
run_case mario-kart-input 3600 kart_start "Mario Kart 64.v64" 45
run_case mario-party-1 2400 press_a_periodically "Mario Party (USA).z64" 50
run_case mario-party-2 2400 press_a_periodically "Mario Party 2 (USA).z64" 50
run_case mario-party-3 2400 press_a_periodically "Mario Party 3 (USA).z64" 50
