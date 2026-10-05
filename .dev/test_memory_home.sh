#!/usr/bin/env bash
# Reuse the leased launcher; HOME pauses are functional, not timing trials.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]:-$0}")/.."
source .dev/env.sh
dol="${1:?usage: test_memory_home.sh <frozen census.dol>}"
chain_file="${2:-$PWD/scripts/chains/memory_home.txt}"
[[ -f "$dol" && -f "${dol%.dol}.elf" ]] || { echo 'Keep the matching DOL and ELF together.' >&2; exit 2; }
if [ -z "${WII_BENCH_JOB:-}" ]; then
    export WII64_CHAIN_FILE="$chain_file" WII64_SKIP_BUILD=1
    source .dev/bench_session.sh memory_home
    bench_default="$HOME/.wii-bench"
    case "$(uname -s)" in Darwin|Linux) ;; *) bench_default=/c/tools/wii-bench ;; esac
    client="${WII_BENCH_CLIENT:-${WII_BENCH_HOME:-$bench_default}/wiibench.py}"
    frozen="$(mktemp -d "$PWD/.dev/runs/memory-home-job-XXXX")"
    cp "$chain_file" "$frozen/chain.txt"
    cp .dev/test_memory_home.sh "$frozen/launcher.sh"
    job="$(python3 "$client" add --name 'Wii64 memory HOME/lent-XFB check' --agent "$WII_BENCH_AGENT" \
        --timeout "$WII64_JOB_TIMEOUT" --cwd "$PWD" -- \
        bash -c 'source="$(< "$1")"; shift; eval "$source"' "$PWD/.dev/test_memory_home.sh" \
        "$frozen/launcher.sh" "$dol" "$frozen/chain.txt")"
    if [ "${WII64_QUEUE_ONLY:-0}" = 1 ]; then echo "$job"; exit 0; fi
    exec python3 "$client" wait "$job"
fi
out="$(mktemp -d "$PWD/.dev/runs/memory-home-XXXX")"
hbc="${WII64_HBC_ROOT:-$PWD/../hbc-reborn}/tools/hbc.py"
WII64_SKIP_BUILD=1 WII64_DOL="$dol" WII64_CHAIN_FILE="$chain_file" WII64_ROM_DIR= \
    bash .dev/hardware_run.sh glN64_wii memory_home > "$out/run.log" 2>&1 &
run_pid=$!
trap 'wait "$run_pid" || true' EXIT # never release the lease while the launcher is live
# The receiver reports the send before the app answers. Never inject into HBC
# or another app; the enclosing queue owns the hardware for this whole test.
ready=0
for _ in {1..90}; do
    version="$(python3 "$hbc" --wii "$WII_BENCH_IP" version 2>/dev/null || true)"
    if [[ "$version" == *' agent' ]] && grep -q '^Sending ' "$out/run.log"; then ready=1; break; fi
    sleep 1
done
if [ "$ready" = 1 ]; then
    sleep 3
    python3 "$hbc" --wii "$WII_BENCH_IP" key h
    sleep 3
    python3 "$hbc" --wii "$WII_BENCH_IP" screen "$out/home.png"
    python3 "$hbc" --wii "$WII_BENCH_IP" key b
fi
wait "$run_pid"
run="$(sed -n 's/^Hardware run complete: //p' "$out/run.log" | tail -n 1)"
python3 - "$run" <<'PY'
import pathlib, sys
log = (pathlib.Path(sys.argv[1]) / 'perf.log').read_text()
assert all('stage=' + stage in log for stage in ('home_enter','home_active','home_closed'))
print('PASS: HOME opened with a lent XFB, recorded active memory, resumed and returned to HBC')
PY
echo "HOME results: $out"
