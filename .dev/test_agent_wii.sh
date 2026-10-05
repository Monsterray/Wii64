#!/usr/bin/env bash
# Expected fatal DSI with a >16 MiB ROM mapped, then automatic HBC return.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"  # toolchain paths; python3 on Windows
cd "$(dirname "${BASH_SOURCE[0]}")/.."
dol="${1:?usage: .dev/test_agent_wii.sh <frozen agent-profiling.dol>}"
[[ -f "$dol" && -f "${dol%.dol}.elf" ]] || { echo "Keep the DOL and matching ELF together." >&2; exit 2; }
if [ -z "${WII_BENCH_JOB:-}" ]; then
    bench_default="$HOME/.wii-bench"
    case "$(uname -s)" in Darwin|Linux) ;; *) bench_default=/c/tools/wii-bench ;; esac
    state="${WII_BENCH_HOME:-$bench_default}"
    client="${WII_BENCH_CLIENT:-$state/wiibench.py}"
    server="${WII_BENCH_SERVER-$(sed -n '1p' "$state/server" 2>/dev/null || true)}"
    [[ -f "$client" && -n "${server//[[:space:]]/}" ]] || { echo "Configure the central Wii queue first." >&2; exit 2; }
    source .dev/bench_session.sh agent_crash
    job="$(python3 "$client" add --name 'Wii64 agent fatal-DSI recovery' --agent "$WII_BENCH_AGENT" --timeout "$WII64_JOB_TIMEOUT" --cwd "$PWD" -- \
        env "WII64_HBC_ROOT=${WII64_HBC_ROOT:-$PWD/../hbc-reborn}" bash "$PWD/.dev/test_agent_wii.sh" "$dol")"
    if [ "${WII64_QUEUE_ONLY:-0}" = 1 ]; then printf '%s\n' "$job"; exit 0; fi
    exec python3 "$client" wait "$job"
fi
out="$(mktemp -d "$PWD/.dev/runs/agent-crash-XXXX")"
result=0
WII64_SKIP_BUILD=1 WII64_DOL="$dol" WII64_CHAIN_FILE="$PWD/scripts/chains/agent_crash.txt" WII64_ROM_DIR= \
    bash .dev/hardware_run.sh glN64_wii agent_crash > "$out/run.log" 2>&1 || result=$?
[[ "$result" != 0 ]] || { echo "Expected crash did not occur; see $out/run.log" >&2; exit 1; }
run="$(sed -n 's/^Diagnostic config: //p' "$out/run.log" | tail -n 1)"
[[ -n "$run" ]] || { tail -n 20 "$out/run.log" >&2; exit 1; }
run="${run%/diag.cfg}"
python3 - "$run" <<'PY'
import json, pathlib, sys
root = pathlib.Path(sys.argv[1])
crash = json.loads((root / 'crash.json').read_text())
assert crash['app'] == 'Wii64' and crash['exception'] == 3 and int(crash['dar'], 16) == 0x10, crash
assert 'devAgent_testCrash' in (root / 'crash.txt').read_text()
print('PASS: fatal DSI delegated with VM active; matching ELF identifies devAgent_testCrash')
PY
hbc_client="${WII64_HBC_ROOT:-$PWD/../hbc-reborn}/tools/hbc.py"
python3 "$hbc_client" --wii "$WII_BENCH_IP" wait 90
echo "Agent crash-recovery run complete: $run"
