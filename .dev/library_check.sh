#!/usr/bin/env bash
# Test the installed ROM collection; freeze inputs, reuse the Dolphin SD profile.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
dol="${1:?usage: library_check.sh <instrumented.dol> [both|dolphin|hardware]}"
mode="${2:-both}"
case "$mode" in both|dolphin|hardware) ;; *) echo 'Use both, dolphin or hardware.' >&2; exit 2 ;; esac
case "$dol" in *Rice*) target=Rice_wii ;; *) target=glN64_wii ;; esac
[[ -f "$dol" && -f "${dol%.dol}.elf" ]] || { echo 'Keep the matching DOL and ELF together.' >&2; exit 2; }
roms="${WII64_ROM_DIR:-$PWD/.dev/dolphin_profile/Load/WiiSDSync/wii64/roms}"
out="$(mktemp -d "$PWD/.dev/runs/library-${target}-$(date +%Y%m%d-%H%M%S)-XXXX")"
python3 - "$dol" "$roms" "$out" "$mode" "$target" <<'PY'
import hashlib, json, pathlib, shutil, subprocess, sys
dol, source, out = map(pathlib.Path, sys.argv[1:4])
mode, target = sys.argv[4:]
if not source.is_dir():
    sys.exit(f'ROM folder not found: {source}; set WII64_ROM_DIR to your owned ROM folder.')
if shutil.disk_usage(out).free < 2 * 1024**3:
    sys.exit('Keep at least 2 GiB free for frozen ROMs and bounded audio captures.')
paths = sorted(p for p in source.iterdir() if p.is_file() and p.suffix.lower() in ('.z64', '.v64', '.n64', '.rom', '.bin'))
if not paths:
    sys.exit('No ROMs found')
if len(paths) > 99:
    sys.exit('Use a collection of at most 99 ROMs')
(out / 'roms').mkdir()
entries = []
for p in paths:
    if any(c in p.name for c in '\n\r|'):
        sys.exit(f'Unsupported ROM filename: {p.name!r}')
    name, vis, replay = p.name, 1800, 'neutral'
    if name == 'Super Mario 64.v64': vis, replay = 2400, 'sm64_start'
    elif name == 'Mario Kart 64.v64': vis, replay = 5400, 'kart_race'
    elif name.startswith('Mario Party'): replay = 'press_a_periodically'
    elif 'Zelda' in name: vis, replay = 9000, 'zelda_new_game'
    line = f'chain={vis},input={replay} sd:/wii64/roms/{name}'
    if len(line.encode()) >= 192:
        sys.exit(f'Diagnostic argument too long: {name}')
    shutil.copy2(p, out / 'roms' / name)
    entries.append(dict(rom=name, vis=vis, replay=replay,
                        sha256=hashlib.sha256(p.read_bytes()).hexdigest(), chain=line))
for suffix in ('.dol', '.elf'):
    shutil.copy2(dol.with_suffix(suffix), out / ('build' + suffix))
manifest = dict(entries=entries, source=str(dol.resolve()),
                build_sha256=hashlib.sha256((out / 'build.dol').read_bytes()).hexdigest(),
                elf_sha256=hashlib.sha256((out / 'build.elf').read_bytes()).hexdigest(),
                target=target, platforms=['dolphin', 'hardware'] if mode == 'both' else [mode],
                launcher_git=subprocess.run(['git', 'rev-parse', 'HEAD'], text=True,
                                            capture_output=True).stdout.strip() or None)
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
(out / 'entries.txt').write_text(''.join(f"{i}|{e['vis']}|{e['chain']}\n" for i,e in enumerate(entries, 1)))
for i in range(0, len(entries), 6):
    (out / f'part_{i//6+1:02d}.txt').write_text(
        'dynacore=dynarec\naudio_quality=accurate\naudio_output=dsp\n'
        'audio_mixer=accurate\naudio_latency=stable\naudio_sync=native\n' +
        '\n'.join(e['chain'] for e in entries[i:i+6]) + '\n')
print(f"Frozen {len(entries)} ROMs and matching build in {out}")
PY
export WII_BENCH_AGENT="wii64-library-$(python3 -c 'import uuid; print(uuid.uuid4().hex)')"
failed=0
if [ "$mode" != dolphin ]; then
    bench_default="$HOME/.wii-bench"
    case "$(uname -s)" in Darwin|Linux) ;; *) bench_default=/c/tools/wii-bench ;; esac
    bench_client="${WII_BENCH_CLIENT:-${WII_BENCH_HOME:-$bench_default}/wiibench.py}"
    for part in "$out"/part_*.txt; do
        job="$(WII64_QUEUE_ONLY=1 WII64_SKIP_BUILD=1 WII64_DOL="$out/build.dol" \
            WII64_CHAIN_FILE="$part" WII64_ROM_DIR="$out/roms" \
            bash .dev/hardware_run.sh "$target" "library_$(basename "$part" .txt)")"
        echo "$job" >> "$out/jobs.txt"
    done
    (
        failed=0
        while IFS= read -r job; do
            status=0
            python3 "$bench_client" wait "$job" \
                > "$out/hardware-$job.log" 2>&1 || status=$?
            run="$(sed -n 's/^Hardware run complete: //p' "$out/hardware-$job.log" | tail -n 1)"
            if [ -z "$run" ]; then
                run="$(sed -n 's/^Diagnostic config: \(.*\)\/diag.cfg$/\1/p' "$out/hardware-$job.log" | tail -n 1)"
            fi
            if [ -n "$run" ] && [ -f "$run/perf.log" ]; then printf '%s\n' "$run" >> "$out/hardware_runs.txt"; fi
            printf '%s %s\n' "$job" "$status" >> "$out/hardware_exits.txt"
            if [ "$status" -ne 0 ]; then failed=1; fi
            echo "Hardware $job: exit $status; log $out/hardware-$job.log"
        done < "$out/jobs.txt"
        exit "$failed"
    ) &
    hardware_wait=$!
fi
if [ "$mode" != hardware ]; then
    while IFS='|' read -r n vis line; do
        log="$out/dolphin-$n.log"
        status=0
        # WAV dump is per ROM; host playback is muted, not the guest engine.
        WII64_ROM_DIR="$out/roms" WII64_DOLPHIN_DSP_HLE=False WII64_DOLPHIN_DUMP_AUDIO=True \
            WII64_DOLPHIN_MUTE_AUDIO=True bash .dev/dolphin_test.sh "$out/build.dol" "$((vis / 25 + 240))" \
            dynacore=dynarec audio_quality=accurate audio_output=dsp audio_mixer=accurate \
            audio_latency=stable audio_sync=native "$line" > "$log" 2>&1 || status=$?
        sed -n 's/^Dolphin chain results: //p' "$log" >> "$out/dolphin_runs.txt"
        printf '%s %s\n' "$n" "$status" >> "$out/dolphin_exits.txt"
        if [ "$status" -ne 0 ]; then failed=1; fi
        echo "Dolphin ROM $n: exit $status; log $log"
    done < "$out/entries.txt"
fi
if [ "$mode" != dolphin ]; then wait "$hardware_wait" || failed=1; fi
python3 scripts/library_report.py "$out" | tee "$out/report.txt"
echo "Library results: $out"
exit "$failed"
