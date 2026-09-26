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
		[[ "$command_line" == *"$PROFILE_POSIX"* ]] && echo "$pid"
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
dump_frames="${WII64_DOLPHIN_DUMP_FRAMES:-False}"
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
	# ROMs and boxart.bin from the one drop folder (see stage_roms.sh).
	"$(dirname "${BASH_SOURCE[0]}")/stage_roms.sh" "$PROFILE_POSIX"
	if [ "$#" -gt 2 ]; then
		printf '%s\n' "${@:3}" > "$LOCAL_WII64/diag.cfg"
		rm -f "$LOCAL_WII64/perf.log" "$LOCAL_WII64"/xfb_*.bin "$LOCAL_WII64"/padtrace_*.csv
	else
		rm -f "$LOCAL_WII64/diag.cfg"
	fi
else
	[ -f "$PROFILE_POSIX/Load/WiiSD.raw" ] || { echo "Raw SD image missing: $PROFILE_POSIX/Load/WiiSD.raw" >&2; exit 1; }
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
		-C Dolphin.Core.WiiSDCard=True \
		-C Dolphin.Core.WiiSDCardAllowWrites=True \
		-C Dolphin.Core.WiiSDCardEnableFolderSync="$folder_sync" \
		-C Dolphin.Interface.ConfirmStop=False \
		-C Logger.Options.WriteToFile=True \
		-C Logger.Logs.MASTER=True \
		-C Logger.Logs.BOOT=True
	echo "Dolphin opened with the isolated Wii64 profile."
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
	-C Dolphin.Core.WiiSDCard=True \
	-C Dolphin.Core.WiiSDCardAllowWrites=True \
	-C Dolphin.Core.WiiSDCardEnableFolderSync="$folder_sync" \
	-C Logger.Options.WriteToFile=True \
	-C Logger.Options.Verbosity=4 \
	-C Logger.Logs.MASTER=True \
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
if [ -f "$frame" ]; then
	echo "Booted OK, frame dumped to: $frame"
else
	if [ "$dump_frames" = True ]; then
		echo "No frame was dumped -- check $PROFILE_POSIX/Logs/dolphin.log" >&2
	fi
fi
tail -20 "$PROFILE_POSIX/Logs/dolphin.log" 2>/dev/null || tail -20 "$PROFILE_POSIX/dolphin-test.log" 2>/dev/null || true
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
if [ -f "$LOCAL_WII64/perf.log" ] && grep -q '^game:' "$LOCAL_WII64/perf.log"; then
	python3 "$(dirname "${BASH_SOURCE[0]}")/../scripts/chain_table.py" "$LOCAL_WII64"
fi
