#!/usr/bin/env bash
# Freeze one probe-capable binary, then queue control/probe/control.
# The optional third argument selects memory; the default is the PC sampler.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh
[[ -z "${WII_BENCH_JOB:-}" ]] || { echo 'Start outside a Wii lease.' >&2; exit 2; }
bench_default="$HOME/.wii-bench"
case "$(uname -s)" in Darwin|Linux) ;; *) bench_default=/c/tools/wii-bench ;; esac
bench_client="${WII_BENCH_CLIENT:-${WII_BENCH_HOME:-$bench_default}/wiibench.py}"
[[ -f "$bench_client" ]] || { echo 'Set up the Wii queue client first.' >&2; exit 2; }
probe="${3:-hprof}"
case "$probe" in hprof|memory) ;; *) echo 'Unknown probe.' >&2; exit 2 ;; esac
if [ "${1:-}" = --collect ]; then
    out="$(cd "${2:?survey directory required}" && pwd)"
    if [ -f "$out/probe" ]; then probe="$(< "$out/probe")"; fi
    python3 - "$out" <<'PY'
import hashlib, json, pathlib, sys
root = pathlib.Path(sys.argv[1])
for name, digest in json.loads((root / 'artifacts.json').read_text()).items():
    if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
        sys.exit('Frozen artifact changed: ' + name)
PY
else
    dol="${1:-wii64-glN64.dol}"
    chain="${2:-pc_sampler}"
    [[ "$chain" =~ ^[A-Za-z0-9_-]+$ && -f "scripts/chains/$chain.txt" ]] || { echo 'Unknown chain.' >&2; exit 2; }
    [[ "$dol" = *.dol && -f "$dol" ]] || { echo 'Pass an existing DOL.' >&2; exit 2; }
    symbol=hprof_entry
    flags=PERF_HPROF
    if [ "$probe" = memory ]; then symbol=perfMem_allocate; flags=PERF_MEMORY; fi
    "$DEVKITPPC/bin/powerpc-eabi-nm" "${dol%.dol}.elf" | grep " T $symbol\$" >/dev/null || {
        echo "Clean-build with PERF_PROF + $flags first." >&2; exit 2; }
    mkdir -p .dev/runs
    out="$(mktemp -d "$PWD/.dev/runs/pc-survey-$(date +%Y%m%d-%H%M%S)-XXXX")"
    cp "$dol" "$out/build.dol"
    cp "${dol%.dol}.elf" "$out/build.elf"
    cp "scripts/chains/$chain.txt" "$out/chain.txt"
    printf '%s\n' "$probe" > "$out/probe"
    git status --short > "$out/source.status"
    git diff --binary HEAD > "$out/source.diff"
    "$DEVKITPPC/bin/powerpc-eabi-gcc" --version > "$out/compiler.txt"
    python3 - "$out" "$probe" <<'PY'
import hashlib, json, pathlib, subprocess, sys
root = pathlib.Path(sys.argv[1])
probe = sys.argv[2]
names = subprocess.check_output(['git', 'ls-files', '-co', '--exclude-standard', '-z']).decode().split('\0')
metadata = dict(head=subprocess.check_output(['git','rev-parse','HEAD']).decode().strip(),
    files_sha256={n:hashlib.sha256(pathlib.Path(n).read_bytes()).hexdigest()
                  for n in sorted(set(names)) if n and pathlib.Path(n).is_file()})
archive = pathlib.Path('.dev/hbc_agent/libhbcagent.a')
if archive.is_file(): metadata['hbc_archive_sha256'] = hashlib.sha256(archive.read_bytes()).hexdigest()
(root/'source.json').write_text(json.dumps(metadata, indent=2)+'\n')
chain = (root/'chain.txt').read_text()
if any(line.startswith(probe+'=') for line in chain.splitlines()):
    sys.exit('The source chain must not set '+probe+'; this driver supplies each mode.')
for label in ('control1','sampler','control2'):
    (root/(label+'.txt')).write_text(chain+'\n'+probe+'='+str(int(label=='sampler'))+'\n')
names = ['build.dol','build.elf','chain.txt','probe','control1.txt','sampler.txt','control2.txt']
(root/'artifacts.json').write_text(json.dumps({n:hashlib.sha256((root/n).read_bytes()).hexdigest()
                                            for n in names}, indent=2)+'\n')
PY
    export WII_BENCH_AGENT="${WII_BENCH_AGENT:-wii64-pc-$(python3 -c 'import uuid; print(uuid.uuid4().hex)')}"
    for label in control1 sampler control2; do
        job="$(WII64_QUEUE_ONLY=1 WII64_SKIP_BUILD=1 WII64_DOL="$out/build.dol" \
            WII64_CHAIN_FILE="$out/$label.txt" bash .dev/hardware_run.sh glN64_wii "pc_$label")"
        printf '%s\n' "$job" > "$out/$label.job"
    done
    echo "$probe survey queued: $out"
    if [ "${WII64_SURVEY_QUEUE_ONLY:-0}" = 1 ]; then
        echo "Collect: bash .dev/profile_pc.sh --collect \"$out\""
        exit 0
    fi
fi
for label in control1 sampler control2; do
    python3 "$bench_client" wait "$(< "$out/$label.job")" --tail 60 | tee "$out/$label.log"
    run="$(sed -n 's/^Hardware run complete: //p' "$out/$label.log" | tail -n 1)"
    [[ -n "$run" && -f "$run/perf.log" ]] || { echo "Missing completed $label result." >&2; exit 1; }
    printf '%s\n' "$run" > "$out/$label.run"
    if [ "$probe" = hprof ]; then
    for profile in "$run"/hprof_*.bin; do
        [[ -f "$profile" ]] || { echo 'Missing sampler/control histogram.' >&2; exit 1; }
        python3 scripts/hprof_view.py "$profile" "$out/build.elf" > "$profile.txt"
    done
    fi
done
if [ "$probe" = memory ]; then
    python3 scripts/memory_report.py "$(< "$out/control1.run")" "$(< "$out/sampler.run")" \
        "$(< "$out/control2.run")" | tee "$out/overhead.txt"
    echo "Memory survey complete: $out"
    exit 0
fi
python3 scripts/hprof_compare.py "$(< "$out/control1.run")" "$(< "$out/sampler.run")" \
    "$(< "$out/control2.run")" | tee "$out/overhead.txt"
echo "PC survey complete: $out"
