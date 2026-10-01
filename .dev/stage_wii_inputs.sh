#!/usr/bin/env bash
# Stage only the named chain's project replays, under the shared Wii lease.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
chain="${1:?usage: stage_wii_inputs.sh <chain name>}"
[[ "$chain" =~ ^[A-Za-z0-9_-]+$ && -f "scripts/chains/$chain.txt" ]] || { echo "Unknown chain: $chain" >&2; exit 2; }
names="$(sed -n 's/^chain=.*[, ]input=\([A-Za-z0-9_-]*\).*/\1/p' "scripts/chains/$chain.txt" | sort -u)"
[[ -n "$names" ]] || { echo "No replays requested by $chain."; exit 0; }
while IFS= read -r name; do
    [[ -s "scripts/inputs/$name.txt" ]] || { echo "Missing replay: $name" >&2; exit 2; }
done <<< "$names"
hbc_root="${WII64_HBC_ROOT:-$PWD/../hbc-reborn}"
[[ -f "$hbc_root/tools/hbc.py" ]] || { echo "Set WII64_HBC_ROOT to the HBC-Reborn checkout." >&2; exit 2; }
if [[ -z "${WII_BENCH_JOB:-}" ]]; then
    bench_default="$HOME/.wii-bench"
    case "$(uname -s)" in Darwin|Linux) ;; *) bench_default=/c/tools/wii-bench ;; esac
    bench_state="${WII_BENCH_HOME:-$bench_default}"
    bench_client="${WII_BENCH_CLIENT:-$bench_state/wiibench.py}"
    bench_server="${WII_BENCH_SERVER-$(sed -n '1p' "$bench_state/server" 2>/dev/null || true)}"
    [[ -f "$bench_client" && -n "${bench_server//[[:space:]]/}" ]] || { echo "Configure the central Wii queue first." >&2; exit 2; }
    job="$(python3 "$bench_client" add --name "Wii64 stage $chain replays" --cwd "$PWD" -- \
        env "WII64_HBC_ROOT=$hbc_root" bash "$PWD/.dev/stage_wii_inputs.sh" "$chain")"
    echo "Queued replay staging: $job."
    exec python3 "$bench_client" wait "$job"
fi
wii_ip="${WII_BENCH_IP:?The dispatcher must supply the leased Wii address.}"
python3 "$hbc_root/tools/hbc.py" --wii "$wii_ip" wait 90
while IFS= read -r name; do
    python3 "$hbc_root/tools/hbc.py" --wii "$wii_ip" put "scripts/inputs/$name.txt" "sd:/wii64/input/$name.txt"
done <<< "$names"
echo "Staged $chain replays; Wii remains in Homebrew Channel."
