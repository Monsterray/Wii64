#!/usr/bin/env bash
# wii64_diag.sh - unattended Dolphin control for diagnosing Wii64, modeled on
# WiiStation's scripts/dolphin_run.sh + wsx.sh (see that project's
# .claude/skills/wiistation-diagnostics/references/unattended-run.md).
# The script is shared; profiles and results under .dev/ stay local.
#
# Every manual step from earlier Wii64 debugging sessions -- launch, find the
# render window, screenshot it, check CPU/responsiveness, close gracefully,
# read the perf log back -- is one subcommand here instead of a fresh
# hand-written PowerShell block each time.
#
# State (current PID + render hwnd) is kept in .dev/.wii64_diag_state so
# `shot`/`status`/`stop` can find the instance `start` launched without it
# being passed around.
#
# Usage:
#   .dev/wii64_diag.sh start <dol> [diag.cfg line]...
#       Stages any diag.cfg lines (see main/main_gc-menu2.cpp's
#       apply_diag_automation doc comment for the autoboot_rom/autonav
#       syntax) into the SD sync folder, launches Dolphin against the shared
#       .dev/dolphin_profile, waits for the window, and records its PID/hwnd.
#       Refuses if an instance is already running against this profile.
#   .dev/wii64_diag.sh shot <out.png>
#       Screenshots the tracked render window via PrintWindow (works even if
#       occluded/backgrounded -- unlike a plain screen copy).
#   .dev/wii64_diag.sh status
#       Prints PID, Responding, and CPU-% over a 3s sample -- CPU pegged +
#       no visible change between two `shot` calls is a hang, not "slow".
#   .dev/wii64_diag.sh stop
#       Closes gracefully (polls for real exit -- this profile's 15MB
#       boxart.bin makes SD folder sync-back take 30-45s, not instant), then
#       prints sd:/wii64/perf.log from the synced-back folder if present, and
#       removes the staged diag.cfg.
#   .dev/wii64_diag.sh killstop
#       Force-kills instead (for a genuinely hung instance where graceful
#       close would just hang too) -- no sync-back, so no perf.log from the
#       usual synced-folder copy, but see `perf` below.
#   .dev/wii64_diag.sh perf [out.log]
#       Force-kills the tracked instance (like killstop), then reads
#       sd:/wii64/perf.log straight out of the raw SD image
#       (dolphin_profile/Load/WiiSD.raw) with scripts/sdimage_read.py,
#       bypassing the graceful-close/sync-back path entirely -- a build's
#       writes land in that image as it runs, a plain kill just needs the
#       OS file lock to release, which is immediate (unlike Dolphin's own
#       clean unmount, which this project's builds don't reliably reach --
#       see doc/subsystem-review.md's shutdown-hang writeup). Needs a
#       PERF_PROF build (.dev/build_profiling.sh) to have anything real in
#       it. Prints the log and, if given, also writes it to out.log.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

PROFILE_WIN='C:\projects\Wii64\.dev\dolphin_profile'
PROFILE_POSIX="$(dirname "${BASH_SOURCE[0]}")/dolphin_profile"
LOCAL_WII64="$PROFILE_POSIX/Load/WiiSDSync/wii64"
DOLPHIN="${DOLPHIN_EXE:-/c/Tools/Dolphin-x64/Dolphin.exe}"
STATE="$(dirname "${BASH_SOURCE[0]}")/.wii64_diag_state"
PS_HELPER="$(dirname "${BASH_SOURCE[0]}")/wii64_diag_helper.ps1"

dolphin_pids() {
	powershell.exe -NoProfile -Command \
		"Get-CimInstance Win32_Process -Filter \"Name='Dolphin.exe'\" | Where-Object { \$_.CommandLine -like '*Wii64*dolphin_profile*' } | Select-Object -ExpandProperty ProcessId" \
		2>/dev/null | tr -d '\r' | grep -E '^[0-9]+$' || true
}

cmd_start() {
	local dol="${1:?usage: start <dol> [diag.cfg line]...}"; shift || true
	local before; before="$(dolphin_pids)"
	if [ -n "$before" ]; then
		echo "A Dolphin is already running against this profile (PID(s): $before) -- refusing to start a second one." >&2
		exit 1
	fi

	"$(dirname "${BASH_SOURCE[0]}")/stage_roms.sh" "$PROFILE_POSIX"
	if [ "$#" -gt 0 ]; then
		printf '%s\n' "$@" > "$LOCAL_WII64/diag.cfg"
		echo "Staged diag.cfg:"; sed 's/^/  /' "$LOCAL_WII64/diag.cfg"
	else
		rm -f "$LOCAL_WII64/diag.cfg"
	fi

	# Stage into .dev/ (no spaces anywhere in this path) rather than pass the
	# caller's path through as-is: a dol under a space-containing path (e.g.
	# the scratchpad dir, "...\Monty Perrotti\...") silently breaks argument
	# passing through the bash -> powershell.exe -File -> Start-Process chain
	# -- Dolphin's main window opens but never loads the dol, and no render
	# window ever appears. Confirmed by testing the identical launch with a
	# short (8.3-style), space-free path, which worked.
	mkdir -p "$(dirname "${BASH_SOURCE[0]}")/run_dol"
	cp "$dol" "$(dirname "${BASH_SOURCE[0]}")/run_dol/current.dol"
	local dol_win="C:\\projects\\Wii64\\.dev\\run_dol\\current.dol"

	powershell.exe -NoProfile -File "$PS_HELPER" -Action start -DolphinExe "$DOLPHIN" -Dol "$dol_win" -Profile "$PROFILE_WIN" -State "$(cd "$(dirname "$STATE")" && pwd -W | tr '/' '\\')\\$(basename "$STATE")"
}

cmd_shot() {
	local out="${1:?usage: shot <out.png>}"
	[ -f "$STATE" ] || { echo "No tracked instance -- run 'start' first." >&2; exit 1; }
	local out_win
	out_win="$(cd "$(dirname "$out")" 2>/dev/null && pwd -W | tr '/' '\\')\\$(basename "$out")" || out_win="$out"
	powershell.exe -NoProfile -File "$PS_HELPER" -Action shot -State "$(cd "$(dirname "$STATE")" && pwd -W | tr '/' '\\')\\$(basename "$STATE")" -Out "$out_win"
}

cmd_status() {
	[ -f "$STATE" ] || { echo "No tracked instance -- run 'start' first." >&2; exit 1; }
	powershell.exe -NoProfile -File "$PS_HELPER" -Action status -State "$(cd "$(dirname "$STATE")" && pwd -W | tr '/' '\\')\\$(basename "$STATE")"
}

cmd_stop() {
	local force="${1:-}"
	[ -f "$STATE" ] || { echo "No tracked instance." >&2; return 0; }
	local pid; pid="$(grep '^PID=' "$STATE" | cut -d= -f2)"
	if [ "$force" = "force" ]; then
		taskkill //F //PID "$pid" >/dev/null 2>&1 || true
	else
		echo "Closing gracefully (can take 30-45s with a full-size boxart.bin)..."
		taskkill //PID "$pid" >/dev/null 2>&1 || true
		for _ in $(seq 1 20); do
			sleep 3
			tasklist //FI "IMAGENAME eq Dolphin.exe" 2>/dev/null | grep -q "$pid" || break
		done
		if tasklist //FI "IMAGENAME eq Dolphin.exe" 2>/dev/null | grep -q "$pid"; then
			echo "Still running after ~60s -- force-killing (no sync-back, so no perf.log from the synced-folder copy below -- use '$0 perf' instead of 'stop' next time to read it straight from the raw SD image)." >&2
			taskkill //F //PID "$pid" >/dev/null 2>&1 || true
		fi
	fi
	rm -f "$STATE"
	if [ -f "$LOCAL_WII64/perf.log" ]; then
		echo "--- $LOCAL_WII64/perf.log ---"
		cat "$LOCAL_WII64/perf.log"
	fi
	rm -f "$LOCAL_WII64/diag.cfg"
}

cmd_perf() {
	local out="${1:-}"
	[ -f "$STATE" ] || { echo "No tracked instance." >&2; exit 1; }
	local pid; pid="$(grep '^PID=' "$STATE" | cut -d= -f2)"
	taskkill //F //PID "$pid" >/dev/null 2>&1 || true
	rm -f "$STATE" "$LOCAL_WII64/diag.cfg"
	sleep 1   # let the OS release the file lock on WiiSD.raw
	local raw="$PROFILE_POSIX/Load/WiiSD.raw"
	[ -f "$raw" ] || { echo "No $raw -- was this instance ever booted with the SD card enabled?" >&2; exit 1; }
	if [ -n "$out" ]; then
		python "$(dirname "${BASH_SOURCE[0]}")/../scripts/sdimage_read.py" "$raw" wii64/perf.log "$out"
		cat "$out"
	else
		python "$(dirname "${BASH_SOURCE[0]}")/../scripts/sdimage_read.py" "$raw" wii64/perf.log
	fi
}

# chain <dol> <chain file> <out dir> [extra diag.cfg lines...]
#   The hardware-session run, in Dolphin: stages the chain file (diag.cfg lines -- chain=
#   per game plus anything else, see scripts/chains/) as diag.cfg, runs Dolphin in batch
#   mode until Wii64 powers off after the last game, then copies diag.cfg, perf.log,
#   padtrace_NN.csv and xfb_NN.bin off the raw SD image into <out dir> and prints the
#   per-game table (scripts/chain_table.py). Time limit: 3x the chain's guest time plus a
#   minute per game; past it the instance is killed and whatever reached the card is read.
#
#   Chains run in a Dolphin profile of their own (.dev/dolphin_runs, made on first use from
#   dolphin_profile's Config; ROMs from stage_roms.sh) with their own state file, so a chain runs beside
#   the user's interactive session instead of refusing to start or killing it -- the split
#   WiiStation made (its commit 28bdac6). The ROMs are copied, not hard-linked: Dolphin's
#   SD sync-back rewrites the sync folder and must not write through to the main profile.
#   WII64_RUNS=<name> picks another such profile (.dev/<name>), for a chain while
#   dolphin_runs itself is open in an interactive window.
RUNS_NAME="${WII64_RUNS:-dolphin_runs}"
RUNS_POSIX="$(dirname "${BASH_SOURCE[0]}")/$RUNS_NAME"
RUNS_WIN="C:\\projects\\Wii64\\.dev\\$RUNS_NAME"
RUNS_WII64="$RUNS_POSIX/Load/WiiSDSync/wii64"
CHAIN_STATE="$(dirname "${BASH_SOURCE[0]}")/.wii64_chain_state_$RUNS_NAME"

runs_pids() {
	powershell.exe -NoProfile -Command \
		"Get-CimInstance Win32_Process -Filter \"Name='Dolphin.exe'\" | Where-Object { \$_.CommandLine -like '*Wii64*$RUNS_NAME -C*' } | Select-Object -ExpandProperty ProcessId" \
		2>/dev/null | tr -d '\r' | grep -E '^[0-9]+$' || true
}

ensure_runs_profile() {
	if [ ! -d "$RUNS_POSIX/Config" ]; then
		mkdir -p "$RUNS_POSIX/Config"
		cp "$PROFILE_POSIX"/Config/*.ini "$RUNS_POSIX/Config/"
	fi
	"$(dirname "${BASH_SOURCE[0]}")/stage_roms.sh" "$RUNS_POSIX"
	[ -e "$RUNS_WII64/RiceVideo.cfg" ] || cp "$PROFILE_POSIX/Load/WiiSDSync/wii64/RiceVideo.cfg" "$RUNS_WII64/" 2>/dev/null || true
}

cmd_chain() {
	local dol="${1:?usage: chain <dol> <chain file> <out dir> [diag lines...]}"
	local chainfile="${2:?chain file}" out="${3:?out dir}"; shift 3
	local before; before="$(runs_pids)"
	if [ -n "$before" ]; then
		echo "A chain is already running (PID(s): $before) -- refusing to start a second one." >&2
		exit 1
	fi
	ensure_runs_profile
	mkdir -p "$out"
	rm -f "$RUNS_WII64"/padtrace_*.csv "$RUNS_WII64"/xfb_*.bin "$RUNS_WII64"/perf.log
	{ tr -d '\r' < "$chainfile" | grep -v '^[[:space:]]*#' | sed '/^[[:space:]]*$/d'; [ "$#" -gt 0 ] && printf '%s\n' "$@"; } > "$RUNS_WII64/diag.cfg"
	cp "$RUNS_WII64/diag.cfg" "$out/diag.cfg"
	local n vis limit
	n=$(grep -c '^chain=' "$RUNS_WII64/diag.cfg")
	vis=$(sed -n 's/^chain=\([0-9]*\)[, ].*/\1/p' "$RUNS_WII64/diag.cfg" | awk '{s+=$1} END {print s+0}')
	limit=$(( vis * 3 / 60 + n * 60 + 120 ))
	echo "Chain: $n game(s), $vis guest VIs, limit ${limit}s"

	mkdir -p "$(dirname "${BASH_SOURCE[0]}")/run_dol"
	cp "$dol" "$(dirname "${BASH_SOURCE[0]}")/run_dol/chain_$RUNS_NAME.dol"
	local state_win; state_win="$(cd "$(dirname "$CHAIN_STATE")" && pwd -W | tr '/' '\\')\\$(basename "$CHAIN_STATE")"
	powershell.exe -NoProfile -File "$PS_HELPER" -Action start -Batch -DolphinExe "$DOLPHIN" \
		-Dol "C:\\projects\\Wii64\\.dev\\run_dol\\chain_$RUNS_NAME.dol" -Profile "$RUNS_WIN" -State "$state_win"
	local pid; pid="$(grep '^PID=' "$CHAIN_STATE" | cut -d= -f2 | tr -d '\r')"
	local t0=$SECONDS how="powered off"
	while tasklist //FI "PID eq $pid" 2>/dev/null | grep -q "$pid"; do
		if [ $((SECONDS - t0)) -ge "$limit" ]; then
			how="KILLED at the ${limit}s limit"
			taskkill //F //PID "$pid" >/dev/null 2>&1 || true
			break
		fi
		sleep 5
	done
	rm -f "$CHAIN_STATE"
	echo "Dolphin exited after $((SECONDS - t0))s ($how)" | tee "$out/run.log"
	sleep 2   # let the OS release WiiSD.raw
	local raw="$RUNS_POSIX/Load/WiiSD.raw" f i
	for f in perf.log $(for i in $(seq 1 "$n"); do printf 'padtrace_%02d.csv xfb_%02d.bin ' "$i" "$i"; done); do
		python "$(dirname "${BASH_SOURCE[0]}")/../scripts/sdimage_read.py" "$raw" "wii64/$f" "$out/$f" >/dev/null 2>&1 || rm -f "$out/$f"
	done
	rm -f "$RUNS_WII64/diag.cfg"
	python "$(dirname "${BASH_SOURCE[0]}")/../scripts/chain_table.py" "$out" | tee -a "$out/run.log"
}

#   record <dol> <rom file name> [vis]
#       Set up a movie recording that scripts/dtm2input.py can turn into a replay: Dolphin
#       opens (profile .dev/dolphin_record, GUI, nothing booted) with the dol as its default
#       game and a one-game chain (default 216000 VIs, an hour) in diag.cfg. Movie > Start
#       Recording Input boots it, so movie frame 0 is power-on and the game autoboots on the
#       same path a chain replay takes. Movie > Stop Recording saves the .dtm; do it before
#       the chain ends. A PERF_PROF dol writes "first_vi: vi0_retrace=N" to perf.log, the
#       --offset this recording needs: dtm2input.py --perf reads it from
#       .dev/dolphin_record/Load/WiiSDSync/wii64/perf.log once Dolphin is closed.
cmd_record() {
	local dol="${1:?usage: record <dol> <rom file name> [vis]}" rom="${2:?rom file name}" vis="${3:-216000}"
	RUNS_NAME=dolphin_record
	RUNS_POSIX="$(dirname "${BASH_SOURCE[0]}")/$RUNS_NAME"
	RUNS_WII64="$RUNS_POSIX/Load/WiiSDSync/wii64"
	CHAIN_STATE="$(dirname "${BASH_SOURCE[0]}")/.wii64_chain_state_$RUNS_NAME"
	[ -z "$(runs_pids)" ] || { echo "A recording Dolphin is already open." >&2; exit 1; }
	ensure_runs_profile
	[ -e "$RUNS_WII64/roms/$rom" ] || { echo "No ROM '$rom' in $RUNS_WII64/roms" >&2; exit 1; }
	rm -f "$RUNS_WII64/perf.log"
	echo "chain=$vis sd:/wii64/roms/$rom" > "$RUNS_WII64/diag.cfg"
	mkdir -p "$(dirname "${BASH_SOURCE[0]}")/run_dol"
	cp "$dol" "$(dirname "${BASH_SOURCE[0]}")/run_dol/record.dol"
	local state_win; state_win="$(cd "$(dirname "$CHAIN_STATE")" && pwd -W | tr '/' '\\')\\$(basename "$CHAIN_STATE")"
	powershell.exe -NoProfile -File "$PS_HELPER" -Action start -Record -DolphinExe "$DOLPHIN" \
		-Dol 'C:\projects\Wii64\.dev\run_dol\record.dol' -Profile "C:\\projects\\Wii64\\.dev\\$RUNS_NAME" -State "$state_win"
	echo "Now: Movie > Start Recording Input (boots Wii64 into $rom), play, Movie > Stop Recording, close Dolphin."
}

case "${1:-}" in
	start)  shift; cmd_start "$@" ;;
	record) shift; cmd_record "$@" ;;
	chain)  shift; cmd_chain "$@" ;;
	shot)   shift; cmd_shot "$@" ;;
	status) cmd_status ;;
	stop)   cmd_stop ;;
	killstop) cmd_stop force ;;
	perf)   shift; cmd_perf "$@" ;;
	*) echo "usage: $0 {start <dol> [diag lines...]|record <dol> <rom file> [vis]|chain <dol> <chain file> <out dir> [diag lines...]|shot <out.png>|status|stop|killstop|perf [out.log]}" >&2; exit 1 ;;
esac
