#!/usr/bin/env bash
# Run the full Wii chain and file a hardware baseline when every game reports.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

id="${1:-$(date +%F)_hw_glN64_full}"
[[ "$id" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid baseline ID: $id" >&2; exit 2; }
[[ ! -e "baselines/$id" ]] || { echo "Baseline already exists: $id" >&2; exit 2; }
grep -qx 'WII64_CHAIN=hardware' .dev/hardware.env || {
	echo "Stage scripts/chains/hardware.txt on the Wii SD card first." >&2
	exit 2
}

mkdir -p .dev/runs
status=".dev/runs/$id.status"
run_log=".dev/runs/$id.hardware.log"
printf 'running\n' > "$status"
trap 'rc=$?; if (( rc == 0 )); then printf "complete\n" > "$status"; else printf "failed (%d)\n" "$rc" > "$status"; fi' EXIT

echo "Building and running the full Wii chain; log: $run_log"
if ! .dev/hardware_run.sh glN64_wii > "$run_log" 2>&1; then
	tail -n 30 "$run_log" >&2
	exit 1
fi
run_dir="$(sed -n 's/^Hardware run complete: //p' "$run_log" | tail -n 1)"
[[ -n "$run_dir" && -f "$run_dir/perf.log" ]] || { echo "Hardware run did not produce perf.log" >&2; exit 1; }
if grep -q '^game:.* how=load_failed' "$run_dir/perf.log"; then
	echo "At least one ROM failed to load; keeping $run_dir without filing a baseline." >&2
	exit 1
fi

python3 scripts/baseline_add.py --chain "$run_dir" --platform hardware --plugin glN64 \
	--id "$id" --purpose "hardware.txt, nine-entry Wii baseline"
echo "Baseline complete: baselines/$id (raw run: $run_dir)"
