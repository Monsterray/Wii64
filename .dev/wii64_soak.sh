#!/usr/bin/env bash
# wii64_soak.sh - one-shot, token-efficient automated boot/hang test.
# The script is shared; profiles and results under .dev/ stay local.
#
# Built after a session that diagnosed an intermittent dynarec hang the slow
# way: launch, sleep N, screenshot, read the image, decide, repeat -- a
# dozen+ round trips per ROM, each one a full tool call plus vision tokens
# to look at a screenshot that usually hadn't changed at all. This collapses
# that into ONE call: it starts Dolphin, polls status+screenshot on its own
# schedule INSIDE this single process, and only needs a human/model to look
# at a screenshot for the FINAL, decisive one -- every intermediate "did
# anything change" question is answered by a file hash, not eyes.
#
# Usage: wii64_soak.sh <dol> <max_seconds> [diag.cfg line]...
#   e.g. wii64_soak.sh wii64-Rice.dol 180 "autoboot_rom=sd:/wii64/roms/Foo.z64" "dynarec_trace=1"
#
# Output: one CSV-ish line per poll (poll#, elapsed_s, cpu_pct, responding,
# changed), ending in a one-line verdict:
#   RUNNING   - still changing/progressing when max_seconds was hit (bump
#               max_seconds and rerun, or just decide it's "slow but alive")
#   STALLED   - screen byte-identical for STABLE_POLLS_TO_STOP consecutive
#               polls AND CPU still busy -- the actual hang signature this
#               tool was built to catch. Worth a look at the last screenshot.
#   IDLE      - screen stable AND CPU dropped to near-idle -- almost always
#               means it settled somewhere at rest (menu, title screen, or
#               the watchdog's own MessageBox) rather than spinning. Still
#               worth a look if you need to know WHICH of those it is.
#   CRASHED   - the tracked Dolphin instance disappeared entirely.
# The instance is left RUNNING when this exits (poll loop ended, not the
# emulator) -- follow up with `wii64_diag.sh stop` or `shot`/`status` as
# normal, same as after using wii64_diag.sh directly.
set -euo pipefail
# Deliberately does NOT cd -- wii64_diag.sh's own relative paths (notably
# its PowerShell helper) are resolved against its BASH_SOURCE, which broke
# when an earlier version of this script cd'd into .dev/ first and then
# invoked it as "./wii64_diag.sh". Referencing it by an absolute path
# (SCRIPT_DIR, computed once via `cd ... && pwd`) avoids the whole problem
# regardless of caller cwd.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DIAG="$SCRIPT_DIR/wii64_diag.sh"

dol="${1:?usage: wii64_soak.sh <dol> <max_seconds> [diag.cfg line]...}"
max_seconds="${2:?usage: wii64_soak.sh <dol> <max_seconds> [diag.cfg line]...}"
shift 2 || true

STABLE_POLLS_TO_STOP=3
IDLE_CPU_THRESHOLD=15   # % of one core -- below this + unchanged screen counts as IDLE, not STALLED
OUTDIR="$SCRIPT_DIR/soak_out/$(date +%s)"
mkdir -p "$OUTDIR"

"$DIAG" start "$dol" "$@"

echo "poll,elapsed_s,cpu_pct,responding,changed"
elapsed=0
prev_hash=""
stable_count=0
poll_num=0
last_shot=""
last_cpu=""
verdict="RUNNING"

while [ "$elapsed" -lt "$max_seconds" ]; do
	interval=15
	if [ "$elapsed" -ge 120 ]; then interval=30; fi
	sleep "$interval"
	elapsed=$((elapsed + interval))
	poll_num=$((poll_num + 1))
	shot="$OUTDIR/poll_${poll_num}.png"
	if ! "$DIAG" shot "$shot" >/dev/null 2>&1; then
		echo "$poll_num,$elapsed,-,-,CRASHED"
		verdict="CRASHED"
		break
	fi
	status_line="$("$DIAG" status 2>&1 || true)"
	cpu="$(echo "$status_line" | grep -oE 'CPU_over_3s=[0-9.]+' | cut -d= -f2)"
	responding="$(echo "$status_line" | grep -oE 'Responding=[A-Za-z]+' | cut -d= -f2)"
	hash="$(powershell.exe -NoProfile -Command "(Get-FileHash '$shot' -Algorithm MD5).Hash" 2>/dev/null | tr -d '\r\n')"
	if [ "$hash" = "$prev_hash" ]; then
		changed="no"; stable_count=$((stable_count + 1))
	else
		changed="yes"; stable_count=0
	fi
	prev_hash="$hash"
	last_shot="$shot"
	last_cpu="$cpu"
	echo "$poll_num,$elapsed,${cpu:--},${responding:--},$changed"
	if [ "$stable_count" -ge "$STABLE_POLLS_TO_STOP" ]; then
		is_idle="$(awk -v c="${cpu:-100}" -v t="$IDLE_CPU_THRESHOLD" 'BEGIN{print (c<t)?"1":"0"}')"
		if [ "$is_idle" = "1" ]; then verdict="IDLE"; else verdict="STALLED"; fi
		break
	fi
done

echo "# verdict: $verdict (last CPU: ${last_cpu:-unknown}%, last screenshot: ${last_shot:-none})"
echo "# instance left running -- .dev/wii64_diag.sh stop|killstop|shot|status as usual"
