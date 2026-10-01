#!/usr/bin/env bash
# Boot a Wii64 .dol in Dolphin for a quick correctness smoke test, the same way
# WiiStation tests its builds. Profiles and results under .dev/ stay local.
#
# Usage: .dev/dolphin_test.sh <path-to.dol> [seconds-to-wait|interactive] [diag.cfg lines...]
#
# Uses a throwaway profile under .dev/ -- never the user's real Dolphin
# profile. Full-frame dumping is opt-in: it can create gigabytes of PNGs.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

DOL="${1:?usage: .dev/dolphin_test.sh <path-to.dol> [seconds-to-wait]}"
WAIT="${2:-25}"
PROFILE_POSIX="${WII64_DOLPHIN_PROFILE:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/dolphin_profile}"
folder_sync="${WII64_DOLPHIN_FOLDER_SYNC:-True}"
dsp_hle="${WII64_DOLPHIN_DSP_HLE:-True}"

case "$(uname -s)" in
	Darwin)
	DOLPHIN="${DOLPHIN_EXE:-/Applications/Dolphin.app/Contents/MacOS/Dolphin}"
	DOLPHIN_APP="${DOLPHIN_APP:-/Applications/Dolphin.app}"
	PROFILE_ARG="$PROFILE_POSIX"
	;;
	*)
	DOLPHIN="${DOLPHIN_EXE:-/c/Tools/Dolphin-x64/Dolphin.exe}"
	PROFILE_WIN='C:\projects\Wii64\.dev\dolphin_profile'
	PROFILE_ARG="$PROFILE_WIN"
	;;
esac

if [ "$(uname -s)" = Darwin ] && pgrep -fl Dolphin 2>/dev/null | grep -F -- " -u $PROFILE_POSIX" >/dev/null; then
	echo "Dolphin is already running with this Wii64 profile. Close it before starting another run." >&2
	exit 1
fi

mac_dolphin_pids() {
	pgrep -f '/Applications/Dolphin.app/Contents/MacOS/Dolphin' 2>/dev/null | while IFS= read -r pid; do
		command_line="$(ps -p "$pid" -o command= 2>/dev/null || true)"
		if [[ "$command_line" == *"$PROFILE_POSIX"* ]]; then echo "$pid"; fi
	done
}

# Only ever look at Dolphin processes launched against THIS profile, matched on
# their command line. Other Dolphin instances (e.g. a concurrent WiiStation test)
# use a different -u profile and must be left strictly alone -- an earlier version
# of this script killed every Dolphin.exe on the box, which would murder them.
windows_dolphin_pids() {
	powershell.exe -NoProfile -Command \
		"Get-CimInstance Win32_Process -Filter \"Name='Dolphin.exe'\" | Where-Object { \$_.CommandLine -like '*Wii64*dolphin_profile*' } | Select-Object -ExpandProperty ProcessId" \
		2>/dev/null | tr -d '\r' | grep -E '^[0-9]+$' || true
}

if [ "$(uname -s)" != Darwin ]; then
	before_pids="$(windows_dolphin_pids)"
	if [ -n "$before_pids" ]; then
		echo "A Dolphin is already running against this profile (PID(s): $(echo "$before_pids" | tr '\n' ' '))." >&2
		exit 1
	fi
fi

mkdir -p "$PROFILE_POSIX"
dolphin_log="$PROFILE_POSIX/Logs/dolphin.log"
max_log_mib="${WII64_DOLPHIN_MAX_LOG_MIB:-64}"
[[ "$max_log_mib" =~ ^[1-9][0-9]*$ ]] || { echo "WII64_DOLPHIN_MAX_LOG_MIB must be positive." >&2; exit 2; }
if ! python3 scripts/dolphin_log.py "$dolphin_log" --max-mib "$max_log_mib"; then
	echo "Close this profile's Dolphin, then gzip its old log before another run." >&2
	exit 1
fi
dump_frames="${WII64_DOLPHIN_DUMP_FRAMES:-False}"
dump_audio="${WII64_DOLPHIN_DUMP_AUDIO:-False}"
default_mute=True
if [ "$WAIT" = interactive ]; then default_mute=False; fi
mute_audio="${WII64_DOLPHIN_MUTE_AUDIO:-$default_mute}"
audio_dump_root="$PROFILE_POSIX/Dump"
if [ "$dump_audio" = True ]; then
	mkdir -p "$PROFILE_POSIX/AudioCaptures"
	audio_dump_root="$(mktemp -d "$PROFILE_POSIX/AudioCaptures/run-XXXXXX")"
fi
if [ "$dump_frames" = True ]; then
	mkdir -p "$PROFILE_POSIX/Dump/Frames"
	rm -f "$PROFILE_POSIX/Dump/Frames/"*.png
fi

if [ "$(uname -s)" = Darwin ]; then
	MAC_WIIMOTE_CONFIG="$HOME/Library/Application Support/Dolphin/Config/WiimoteNew.ini"
	if [ -f "$MAC_WIIMOTE_CONFIG" ]; then
		mkdir -p "$PROFILE_POSIX/Config"
		cp "$MAC_WIIMOTE_CONFIG" "$PROFILE_POSIX/Config/WiimoteNew.ini"
		echo "Using the macOS Dolphin Wii Remote and mouse bindings."
	fi
fi

LOCAL_WII64="$PROFILE_POSIX/Load/WiiSDSync/wii64"
raw="$PROFILE_POSIX/Load/WiiSD.raw"
mkdir -p "$LOCAL_WII64"
if [ "$folder_sync" = True ]; then
	[ ! -e "$PROFILE_POSIX/Load/WiiSDSync.xxx" ] || {
		echo "Dolphin left an SD sync backup: $PROFILE_POSIX/Load/WiiSDSync.xxx" >&2
		echo "With this profile stopped, preserve that backup under .dev/runs/ and recover missing files before retrying." >&2
		exit 1
	}
	# ROMs and boxart.bin from the one drop folder (see stage_roms.sh).
	"$(dirname "${BASH_SOURCE[0]}")/stage_roms.sh" "$PROFILE_POSIX"
	if [ "$#" -gt 2 ]; then
		printf '%s\n' "${@:3}" > "$LOCAL_WII64/diag.cfg"
		python3 - "$LOCAL_WII64" "${@:3}" <<'PY'
import pathlib, sys
root = pathlib.Path(sys.argv[1])
missing = []
for line in sys.argv[2:]:
    if not line.startswith('chain='):
        continue
    rom = line.split(' ', 1)[1]
    if not rom.startswith('sd:/wii64/') or not (root / rom[len('sd:/wii64/'):]).is_file():
        missing.append(rom)
if missing:
    sys.exit('Missing test ROMs (stage them before boot):\n' + '\n'.join(missing))
PY
		rm -f "$LOCAL_WII64/perf.log" "$LOCAL_WII64"/xfb_*.bin "$LOCAL_WII64"/padtrace_*.csv
	else
		rm -f "$LOCAL_WII64/diag.cfg"
	fi
else
	[ -f "$PROFILE_POSIX/Load/WiiSD.raw" ] || { echo "Raw SD image missing: $PROFILE_POSIX/Load/WiiSD.raw" >&2; exit 1; }
	[ "$#" -le 2 ] || { echo "Diagnostic arguments require WII64_DOLPHIN_FOLDER_SYNC=True; raw mode uses the config already in WiiSD.raw." >&2; exit 2; }
	# These are extracted copies, not live writes. An earlier chain's log
	# must not make a new raw-image run appear complete before its first VI.
	rm -f "$LOCAL_WII64/perf.log" "$LOCAL_WII64"/xfb_*.bin "$LOCAL_WII64"/padtrace_*.csv
	if ! python3 scripts/sdimage_read.py "$PROFILE_POSIX/Load/WiiSD.raw" wii64/diag.cfg "$LOCAL_WII64/diag.cfg" >/dev/null 2>&1; then
		rm -f "$LOCAL_WII64/diag.cfg"
	fi
	echo "Using the pre-staged raw Dolphin SD image (folder sync is off)."
fi

if [ "$(uname -s)" = Darwin ]; then
	DOL_ARG="$(cd "$(dirname "$DOL")" && pwd)/$(basename "$DOL")"
else
	DOL_ARG="$(cd "$(dirname "$DOL")" && pwd -W | tr '/' '\\')\\$(basename "$DOL")"
fi

if [ "$(uname -s)" = Darwin ] && [ "$WAIT" = interactive ]; then
	open -n -a /Applications/Dolphin.app --args -e "$DOL_ARG" -u "$PROFILE_ARG" \
		-C Dolphin.Core.MMU=True \
		-C Dolphin.Core.DSPHLE="$dsp_hle" \
		-C Dolphin.DSP.DumpAudio="$dump_audio" \
		-C Dolphin.DSP.Muted="$mute_audio" \
		-C Dolphin.General.DumpPath="$audio_dump_root" \
		-C Dolphin.Core.WiiSDCard=True \
		-C Dolphin.Core.WiiSDCardAllowWrites=True \
		-C Dolphin.Core.WiiSDCardEnableFolderSync="$folder_sync" \
		-C Dolphin.Interface.ConfirmStop=False \
		-C Logger.Options.WriteToFile=True \
		-C Logger.Logs.MASTER=True \
		-C Logger.Logs.BOOT=True
	echo "Dolphin opened with the isolated Wii64 profile."
	if [ "$dump_audio" = True ]; then echo "Audio capture will be under $audio_dump_root"; fi
	exit 0
fi

args=(-b -e "$DOL_ARG" -u "$PROFILE_ARG" \
	-C Dolphin.Interface.ConfirmStop=False \
	-C Dolphin.Interface.OnScreenDisplayMessages=False \
	-C Dolphin.Interface.UsePanicHandlers=False \
	-C Dolphin.Analytics.PermissionAsked=True \
	-C Dolphin.Analytics.Enabled=False \
	-C Dolphin.Movie.DumpFrames="$dump_frames" \
	-C Graphics.Settings.DumpFramesAsImages="$dump_frames" \
	-C Graphics.Settings.PNGCompressionLevel=1 \
	-C Graphics.Hacks.ImmediateXFBEnable=True \
	-C Dolphin.Core.CPUThread=True \
	-C Dolphin.Core.MMU=True \
	-C Dolphin.Core.DSPHLE="$dsp_hle" \
	-C Dolphin.DSP.DumpAudio="$dump_audio" \
	-C Dolphin.DSP.Muted="$mute_audio" \
	-C Dolphin.General.DumpPath="$audio_dump_root" \
	-C Dolphin.Core.WiiSDCard=True \
	-C Dolphin.Core.WiiSDCardAllowWrites=True \
	-C Dolphin.Core.WiiSDCardEnableFolderSync="$folder_sync" \
	-C Logger.Options.WriteToFile=True \
	-C Logger.Options.Verbosity=4 \
	-C Logger.Logs.MASTER=True \
	-C Logger.Logs.COMMON=True \
	-C Logger.Logs.BOOT=True)
if [ "$(uname -s)" = Darwin ]; then
	open -n -a "$DOLPHIN_APP" --args "${args[@]}" >"$PROFILE_POSIX/dolphin-test.log" 2>&1
	dolphin_pid=""
	for _ in {1..15}; do
		dolphin_pid="$(mac_dolphin_pids | tr '\n' ' ')"
		[ -n "$dolphin_pid" ] && break
		sleep 1
	done
	[ -n "$dolphin_pid" ] || { echo "Dolphin did not start with profile $PROFILE_POSIX" >&2; exit 1; }
	set -- $dolphin_pid
	tracked_pids=("$@")
else
	"$DOLPHIN" "${args[@]}" >"$PROFILE_POSIX/dolphin-test.log" 2>&1 &
	tracked_pids=("$!")
fi

echo "Booting $DOL in Dolphin, waiting ${WAIT}s..."
elapsed=0
test_status=0
chain_count=0
if [ -f "$LOCAL_WII64/diag.cfg" ]; then chain_count="$(grep -c '^chain=' "$LOCAL_WII64/diag.cfg" || true)"; fi
raw_ready=1
if [ "$chain_count" -gt 0 ] && [ -f "$raw" ]; then
	old_games="$(python3 scripts/sdimage_read.py "$raw" wii64/perf.log 2>/dev/null | tr -d '\000' | grep -c '^game: ' || true)"
	[ "$old_games" -eq 0 ] || raw_ready=0
fi
while [ "$elapsed" -lt "$WAIT" ]; do
	if [ "$(uname -s)" = Darwin ]; then
		[ -n "$(mac_dolphin_pids)" ] || break
	else
		kill -0 "${tracked_pids[0]}" 2>/dev/null || break
	fi
	if [ $((elapsed % 5)) -eq 0 ] && ! python3 scripts/dolphin_log.py "$dolphin_log" --max-mib "$max_log_mib"; then
		echo "Stopping this test profile to prevent a runaway warning log. Diagnostics are retained." >&2
		test_status=1
		break
	fi
	if [ "$chain_count" -gt 0 ]; then
		game_count=0
		if [ -s "$LOCAL_WII64/perf.log" ]; then
			game_count="$(tr -d '\000' < "$LOCAL_WII64/perf.log" | grep -c '^game: ' || true)"
		elif { [ "$raw_ready" -eq 0 ] || [ $((elapsed % 5)) -eq 0 ]; } && [ -f "$raw" ]; then
			game_count="$(python3 scripts/sdimage_read.py "$raw" wii64/perf.log 2>/dev/null | tr -d '\000' | grep -c '^game: ' || true)"
			if [ "$raw_ready" -eq 0 ]; then
				[ "$game_count" -ne 0 ] || raw_ready=1
				game_count=0
			fi
		fi
		if [ "$game_count" -ge "$chain_count" ]; then
			echo "Dolphin recorded all $game_count/$chain_count game results."
			sleep 2
			break
		fi
	fi
	sleep 1; elapsed=$((elapsed + 1))
done

if [ "$(uname -s)" = Darwin ]; then
	# Stop only the separate app instance using this profile.
	if [ -n "$(mac_dolphin_pids)" ]; then
		for p in "${tracked_pids[@]}"; do kill -TERM "$p" 2>/dev/null || true; done
		for _ in {1..15}; do
			[ -z "$(mac_dolphin_pids)" ] && break
			sleep 1
		done
		if [ -n "$(mac_dolphin_pids)" ]; then
			echo "Dolphin did not stop cleanly; closing only the process using this test profile." >&2
			for p in "${tracked_pids[@]}"; do kill -KILL "$p" 2>/dev/null || true; done
		fi
		for _ in {1..10}; do
			[ -z "$(mac_dolphin_pids)" ] && break
			sleep 1
		done
		[ -z "$(mac_dolphin_pids)" ] || { echo "Dolphin is still running with profile $PROFILE_POSIX" >&2; exit 1; }
	fi
	dolphin_status=0
else
	dolphin_status=0
	for p in "${tracked_pids[@]}"; do
		taskkill //F //PID "$p" >/dev/null 2>&1 || true
	done
fi

frame="$PROFILE_POSIX/Dump/Frames/framedump_1.png"
if [ "$dump_frames" = True ] && [ -f "$frame" ]; then
	echo "Booted OK, frame dumped to: $frame"
else
	if [ "$dump_frames" = True ]; then
		echo "No frame was dumped -- check $PROFILE_POSIX/Logs/dolphin.log" >&2
	fi
fi
if [ "$dump_audio" = True ]; then
	python3 scripts/audio_capture_report.py "$audio_dump_root"
fi
if [ -f "$PROFILE_POSIX/Logs/dolphin.log" ]; then
	# Dolphin appends boots to this log; report only the run just launched.
	python3 scripts/dolphin_log.py "$dolphin_log"
	python3 scripts/dolphin_log.py "$dolphin_log" --check-faults || test_status=1
else
	tail -20 "$PROFILE_POSIX/dolphin-test.log" 2>/dev/null || true
fi
[ "$dolphin_status" -eq 0 ] || exit 1
if [ "$folder_sync" = False ] || { [ "$chain_count" -gt 0 ] && [ ! -s "$LOCAL_WII64/perf.log" ]; }; then
	for name in diag.cfg perf.log; do
		rm -f "$LOCAL_WII64/$name"
		python3 "$(dirname "${BASH_SOURCE[0]}")/../scripts/sdimage_read.py" "$raw" "wii64/$name" "$LOCAL_WII64/$name" >/dev/null 2>&1 || rm -f "$LOCAL_WII64/$name"
	done
	frame_count="$chain_count"
	[ "$frame_count" -gt 0 ] || frame_count=9
	for n in $(seq -w 1 "$frame_count"); do
		for name in "xfb_$n.bin" "padtrace_$n.csv"; do
			rm -f "$LOCAL_WII64/$name"
			python3 "$(dirname "${BASH_SOURCE[0]}")/../scripts/sdimage_read.py" "$raw" "wii64/$name" "$LOCAL_WII64/$name" >/dev/null 2>&1 || rm -f "$LOCAL_WII64/$name"
		done
	done
fi
# A menu-only boot can leave the previous chain's log in the reused profile.
if [ "$chain_count" -gt 0 ] && [ -f "$LOCAL_WII64/perf.log" ] && grep -q '^game:' "$LOCAL_WII64/perf.log"; then
	python3 "$(dirname "${BASH_SOURCE[0]}")/../scripts/chain_table.py" "$LOCAL_WII64"
	# The profile is reused. Retain this chain before the next launch overwrites it.
	mkdir -p .dev/runs
	out="$(mktemp -d "$PWD/.dev/runs/dolphin-chain-$(date +%Y%m%d-%H%M%S)-XXXX")"
	for name in diag.cfg perf.log; do cp "$LOCAL_WII64/$name" "$out/$name"; done
	for file in "$LOCAL_WII64"/padtrace_*.csv "$LOCAL_WII64"/xfb_*.bin; do
		[ ! -f "$file" ] || cp "$file" "$out/"
	done
	python3 - "$DOL" "$out" <<'PY'
import hashlib, json, pathlib, sys
dol, out = map(pathlib.Path, sys.argv[1:])
manifest = {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in (dol, dol.with_suffix('.elf')) if p.is_file()}
(out / 'artifacts.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
	python3 scripts/check_hardware_run.py "$out" --dolphin || test_status=1
	python3 scripts/subsystem_report.py "$out" > "$out/subsystems.txt"
	if [ -f "$dolphin_log" ]; then
		python3 scripts/dolphin_log.py "$dolphin_log" --check-faults > "$out/fault-check.txt" || test_status=1
	fi
	echo "Dolphin chain results: $out"
fi
exit "$test_status"
