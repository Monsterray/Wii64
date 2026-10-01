#!/usr/bin/env bash
# Freeze full/control builds, queue a Wii A/B/A survey, then compare results.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
[[ -z "${WII_BENCH_JOB:-}" ]] || { echo "Start this survey outside a Wii lease." >&2; exit 2; }
chain="${1:-subsystem_survey}"
resume=""
if [ "$chain" = --collect ]; then
    resume="${2:?usage: .dev/profile_subsystems.sh --collect <survey-directory>}"
    shift 2
    [[ "$#" = 0 ]] || { echo "--collect takes only the survey directory." >&2; exit 2; }
    chain=subsystem_survey
elif [ "$#" -gt 0 ]; then
    shift
fi
[[ "$chain" =~ ^[A-Za-z0-9_-]+$ && -f "scripts/chains/$chain.txt" ]] || { echo "Unknown chain: $chain" >&2; exit 2; }
bench_default="$HOME/.wii-bench"
case "$(uname -s)" in Darwin|Linux) ;; *) bench_default=/c/tools/wii-bench ;; esac
bench_state="${WII_BENCH_HOME:-$bench_default}"
bench_client="${WII_BENCH_CLIENT:-$bench_state/wiibench.py}"
bench_server="${WII_BENCH_SERVER-$(sed -n '1p' "$bench_state/server" 2>/dev/null || true)}"
[[ -f "$bench_client" && -n "${bench_server//[[:space:]]/}" ]] || { echo "Set up wiibench.py with the central server first." >&2; exit 2; }
mkdir -p .dev/runs
if [ -n "$resume" ]; then
out="$(cd "$resume" && pwd)"
python3 - "$out" <<'PY'
import hashlib, json, pathlib, sys
root = pathlib.Path(sys.argv[1])
for name, digest in json.loads((root / 'artifacts.json').read_text()).items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest, name
for label in ('full1', 'control', 'full2'):
    assert (root / (label + '.job')).read_text().strip(), label
PY
else
out="$(mktemp -d "$PWD/.dev/runs/subsystem-survey-$(date +%Y%m%d-%H%M%S)-XXXX")"
echo "Survey artifacts and logs: $out"
source .dev/env.sh
git status --short > "$out/source.status"
"$DEVKITPPC/bin/powerpc-eabi-gcc" --version > "$out/compiler.txt"
python3 - "$out" "$@" <<'PY'
import hashlib, json, os, pathlib, subprocess, sys
names = subprocess.check_output(['git', 'ls-files', '-co', '--exclude-standard', '-z']).decode().split('\0')
hashes = {name: hashlib.sha256(pathlib.Path(name).read_bytes()).hexdigest()
          for name in sorted(set(names)) if name and pathlib.Path(name).is_file()}
metadata = {
    'head': subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode().strip(),
    'make_args': sys.argv[2:], 'files_sha256': hashes,
    'same_probes': os.environ.get('WII64_SURVEY_SAME_PROBES') == '1'}
if 'HBC_AGENT=1' in sys.argv[2:]:
    sdk = pathlib.Path(next((arg.split('=', 1)[1] for arg in sys.argv[2:]
                             if arg.startswith('HBC_AGENT_ROOT=')), '../hbc-reborn'))
    metadata['hbc_sdk'] = {
        'head': subprocess.check_output(['git', '-C', str(sdk), 'rev-parse', 'HEAD']).decode().strip(),
        'status': subprocess.check_output(['git', '-C', str(sdk), 'status', '--short']).decode(),
        'archive_sha256': hashlib.sha256((sdk / 'sdk/hbc_agent/libogc2/libhbcagent.a').read_bytes()).hexdigest()}
pathlib.Path(sys.argv[1], 'source.json').write_text(json.dumps(metadata, indent=2) + '\n')
PY
for mode in full control; do
    flags=-DPERF_PROF
    if [ "$mode" = full ]; then flags+=' -DPERF_SUBSYSTEM_PROBES'; fi
    if [ "$mode" = full ]; then flags="${WII64_SURVEY_FULL_FLAGS:-$flags}";
    else flags="${WII64_SURVEY_CONTROL_FLAGS:-$flags}"; fi
    printf '%s\n' "$flags" > "$out/$mode.flags"
    echo "Clean build: $mode (see $out/$mode.build.log)"
    if ! .dev/build_profiling.sh glN64_wii "DEBUG_FLAGS=$flags" "$@" > "$out/$mode.build.log" 2>&1; then
        tail -n 30 "$out/$mode.build.log" >&2
        exit 1
    fi
    cp wii64-glN64.dol "$out/$mode.dol"
    cp wii64-glN64.elf "$out/$mode.elf"
done
python3 - "$out" <<'PY'
import hashlib, json, pathlib, sys
root = pathlib.Path(sys.argv[1])
files = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
         for p in root.iterdir() if p.suffix in ('.dol', '.elf')}
(root / 'artifacts.json').write_text(json.dumps(files, indent=2) + '\n')
PY
for run in full1 control full2; do
    mode=full
    if [ "$run" = control ]; then mode=control; fi
    job="$(WII64_QUEUE_ONLY=1 WII64_SKIP_BUILD=1 WII64_DOL="$out/$mode.dol" \
        bash .dev/hardware_run.sh glN64_wii "$chain")"
    printf '%s\n' "$job" > "$out/$run.job"
done
if [ "${WII64_SURVEY_QUEUE_ONLY:-0}" = 1 ]; then
    echo "Survey queued; frozen artifacts and job IDs: $out"
    echo "Collect later: bash .dev/profile_subsystems.sh --collect \"$out\""
    exit 0
fi
fi
for run in full1 control full2; do
    job="$(< "$out/$run.job")"
    echo "Waiting for $run job $job; other workstations keep their queue turns."
    python3 "$bench_client" wait "$job" --tail 60 | tee "$out/$run.log"
    run_dir="$(sed -n 's/^Hardware run complete: //p' "$out/$run.log" | tail -n 1)"
    [[ -n "$run_dir" && -f "$run_dir/perf.log" ]] || { echo "No completed results for $job." >&2; exit 1; }
    printf '%s\n' "$run_dir" > "$out/$run.run"
    python3 scripts/subsystem_report.py "$run_dir" > "$out/$run.report.txt"
done
compare_args=(scripts/subsystem_compare.py)
same_probes="$(python3 -c 'import json,sys; print(int(json.load(open(sys.argv[1])).get("same_probes", False)))' "$out/source.json")"
if [ "$same_probes" = 1 ]; then compare_args+=(--same-probes); fi
python3 "${compare_args[@]}" "$(< "$out/full1.run")" "$(< "$out/control.run")" \
    "$(< "$out/full2.run")" | tee "$out/overhead.txt"
echo "Survey complete: $out"
