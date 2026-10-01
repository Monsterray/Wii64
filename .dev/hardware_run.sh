#!/usr/bin/env bash
# Build, launch through Homebrew Channel, and collect one Wii hardware run.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]:-$0}")/.."

# Every workstation must hold the central lease before it contacts the Wii.
if [ -z "${WII_BENCH_JOB:-}" ]; then
	bench_default="$HOME/.wii-bench"
	if [ "$(uname -s)" != Darwin ] && [[ "$(uname -s)" != Linux ]]; then bench_default=/c/tools/wii-bench; fi
	bench_state="${WII_BENCH_HOME:-$bench_default}"
	bench_client="${WII_BENCH_CLIENT:-$bench_state/wiibench.py}"
	[[ -f "$bench_client" ]] || { echo "Set up the workstation's wiibench.py client first; see doc/hardware-session.md." >&2; exit 2; }
	bench_server="${WII_BENCH_SERVER-$(sed -n '1p' "$bench_state/server" 2>/dev/null || true)}"
	[[ -n "${bench_server//[[:space:]]/}" ]] || { echo "Configure the central lease server with wiibench.py setup --server URL before testing." >&2; exit 2; }
	queue_chain="${2:-$(sed -n 's/^WII64_CHAIN=//p' .dev/hardware.env | tail -n 1)}"
	source .dev/bench_session.sh "$queue_chain"
	job_env=("WII64_SKIP_BUILD=${WII64_SKIP_BUILD:-0}" "WII64_STAGE_INPUTS=${WII64_STAGE_INPUTS:-1}")
	job_env+=("WII64_RECEIVER_TIMEOUT=$WII64_RECEIVER_TIMEOUT" "WII64_JOB_TIMEOUT=$WII64_JOB_TIMEOUT")
	if [ -n "${WII64_DOL:-}" ]; then job_env+=("WII64_DOL=$WII64_DOL"); fi
	if [ -n "${WII64_CHAIN_FILE:-}" ]; then job_env+=("WII64_CHAIN_FILE=$WII64_CHAIN_FILE"); fi
	if [ -n "${WII64_ROM_DIR:-}" ]; then job_env+=("WII64_ROM_DIR=$WII64_ROM_DIR"); fi
	if [ -n "${WII64_HBC_ROOT:-}" ]; then job_env+=("WII64_HBC_ROOT=$WII64_HBC_ROOT"); fi
	job="$(python3 "$bench_client" add --name "Wii64 ${1:-glN64_wii} ${2:-configured chain}" --agent "$WII_BENCH_AGENT" --timeout "$WII64_JOB_TIMEOUT" --cwd "$PWD" -- \
		env "${job_env[@]}" bash -c "$(< "$PWD/.dev/hardware_run.sh")" "$PWD/.dev/hardware_run.sh" "$@")"
	if [ "${WII64_QUEUE_ONLY:-0}" = 1 ]; then
		echo "Queued Wii64 hardware run: $job." >&2
		printf '%s\n' "$job"
		exit 0
	fi
	echo "Queued Wii64 hardware run: $job. Waiting for the central lease and Homebrew Channel."
	exec python3 "$bench_client" wait "$job"
fi
source .dev/env.sh

config=.dev/hardware.env
[ -f "$config" ] || { echo "Run .dev/hardware_setup.sh first." >&2; exit 1; }
value() { sed -n "s/^$1=//p" "$config" | tail -n 1; }
wii_ip="${WII_BENCH_IP:-$(value WII64_WII_IP)}"
mac_ip="$(value WII64_MAC_IP)"
python3 -c 'import ipaddress,sys; [ipaddress.IPv4Address(x) for x in sys.argv[1:]]' "$wii_ip" "$mac_ip"
python3 -c 'import socket,sys; s=socket.socket(); s.bind((sys.argv[1],0)); s.close()' "$mac_ip" || { echo "This Mac cannot bind to $mac_ip; check .dev/hardware.env." >&2; exit 1; }
# A bare connect-and-close can stall older HBC's loader; send a rejected header.
for attempt in 1 2 3 4 5; do
	if python3 -c 'import socket,sys; s=socket.create_connection((sys.argv[1],4299),3); s.sendall(b"PING"+bytes(12)); s.close()' "$wii_ip" 2>/dev/null; then break; fi
	if [ "$attempt" = 5 ]; then
		echo "Homebrew Channel upload port is not reachable at $wii_ip:4299; check its network icon and IP." >&2
		exit 1
	fi
	echo "Waiting for Homebrew Channel network ($attempt/5)..."
	sleep 2
done
chain="${2:-$(value WII64_CHAIN)}"
chain_file="${WII64_CHAIN_FILE:-scripts/chains/$chain.txt}"
[[ "$chain" =~ ^[A-Za-z0-9_-]+$ && -f "$chain_file" ]] || {
	echo "Unknown chain '$chain'; choose a file in scripts/chains/ without its .txt suffix." >&2
	exit 2
}
command -v wiiload >/dev/null || { echo "wiiload is missing from devkitPro tools/bin." >&2; exit 1; }

target="${1:-glN64_wii}"
case "$target" in
	glN64_wii) dol=wii64-glN64.dol ;;
	Rice_wii) dol=wii64-Rice.dol ;;
	*) echo "Use glN64_wii or Rice_wii." >&2; exit 2 ;;
esac
if [ "${WII64_SKIP_BUILD:-0}" != 1 ]; then
	case "$chain" in
	mario_kart_warm|mario_kart_race|mario_titles_warm)
		.dev/build_profiling.sh "$target" 'DEBUG_FLAGS=-DPERF_PROF -DPERF_SUBSYSTEM_PROBES'
		;;
	*)
		.dev/build_profiling.sh "$target"
		;;
	esac
fi
dol="${WII64_DOL:-$dol}"
[[ -f "$dol" ]] || { echo "Missing $dol; build it or unset WII64_SKIP_BUILD=1." >&2; exit 1; }
mkdir -p .dev/runs
out="$(mktemp -d ".dev/runs/hardware-${target}-$(date +%Y%m%d-%H%M%S)-XXXX")"
cp "$chain_file" "$out/diag.cfg"
printf 'result_host=%s\n' "$mac_ip" >> "$out/diag.cfg"
if [ -n "${WII64_ROM_DIR:-}" ]; then
	[ -d "$WII64_ROM_DIR" ] || { echo "ROM folder does not exist: $WII64_ROM_DIR" >&2; exit 1; }
	printf 'rom_fetch=1\n' >> "$out/diag.cfg"
fi
python3 - "$out" "$dol" <<'PY'
import hashlib, json, pathlib, sys
out, dol = map(pathlib.Path, sys.argv[1:])
paths = [dol, dol.with_suffix('.elf'), out / 'diag.cfg']
hashes = {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
(out / 'artifacts.json').write_text(json.dumps(hashes, indent=2) + '\n')
PY
receiver_python=python3
if [ "$(uname -s)" = Darwin ] && [ -x /usr/bin/python3 ]; then receiver_python=/usr/bin/python3; fi
source .dev/bench_session.sh "$chain"
receiver_timeout="$WII64_RECEIVER_TIMEOUT"
receiver_cmd=("$receiver_python" scripts/hardware_receive.py "$out" --bind "$mac_ip" --wii-ip "$wii_ip" --timeout "$receiver_timeout")
if [ -n "${WII64_ROM_DIR:-}" ]; then receiver_cmd+=(--rom-dir "$WII64_ROM_DIR"); fi
hbc_root="${WII64_HBC_ROOT:-$PWD/../hbc-reborn}"
hbc_client="$hbc_root/tools/hbc.py"
if [ -f "$hbc_client" ]; then
	# Port 4299 also answers inside an app: require HBC itself before uploading.
	python3 "$hbc_client" --wii "$wii_ip" wait 90
	python3 "$hbc_client" --wii "$wii_ip" --json status > "$out/hbc-before.json"
	if [ "${WII64_STAGE_INPUTS:-1}" = 1 ]; then
		bash .dev/stage_wii_inputs.sh "$chain"
	fi
	receiver_cmd+=(--hbc-client "$hbc_client")
	elf="${dol%.dol}.elf"
	if [ -f "$elf" ]; then receiver_cmd+=(--elf "$elf"); fi
fi
"${receiver_cmd[@]}" >"$out/receiver.log" 2>&1 &
receiver=$!
cleanup() { kill "$receiver" 2>/dev/null || true; }
trap cleanup EXIT
for _ in {1..40}; do
	[ -f "$out/.ready" ] && break
	kill -0 "$receiver" 2>/dev/null || { sed -n '1,20p' "$out/receiver.log" >&2; exit 1; }
	sleep 0.25
done
[ -f "$out/.ready" ] || { echo "Result receiver did not start; see $out/receiver.log" >&2; exit 1; }
echo "Sending $dol to Wii $wii_ip. Keep Homebrew Channel open; the run returns there when complete."
echo "Diagnostic config: $out/diag.cfg"
diag_args=()
while IFS= read -r line; do
	[[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
	((${#line} < 192)) || { echo "Diagnostic line exceeds the Wii limit of 191 bytes." >&2; exit 1; }
	if [[ "$line" =~ ,input=([A-Za-z0-9_-]+) ]] && [ ! -f "scripts/inputs/${BASH_REMATCH[1]}.txt" ]; then
		echo "Missing replay scripts/inputs/${BASH_REMATCH[1]}.txt; stage replays on SD with .dev/hardware_setup.sh." >&2
		exit 1
	fi
	diag_args+=("--diag=$line")
done < "$out/diag.cfg"
((${#diag_args[@]} <= 32)) || { echo "The Wii accepts at most 32 diagnostic lines." >&2; exit 1; }
WIILOAD="tcp:$wii_ip" wiiload "$dol" "${diag_args[@]}"
touch "$out/.sent"
echo "Waiting for the Wii to finish; results will arrive in $out"
wait "$receiver"
trap - EXIT
python3 scripts/chain_table.py "$out" | tee "$out/summary.txt"
echo "Waiting for Homebrew Channel to return..."
if [ -f "$hbc_client" ]; then
python3 "$hbc_client" --wii "$wii_ip" wait 90
python3 "$hbc_client" --wii "$wii_ip" --json status > "$out/hbc-after.json"
python3 - "$hbc_client" "$out" "$elf" <<'PY'
import json, pathlib, sys
from scripts.hbc_watch import RunWatch, load_client
out = pathlib.Path(sys.argv[2])
watch = RunWatch(load_client(sys.argv[1]), '', out, sys.argv[3])
failure = watch.new_crash(json.loads((out / 'hbc-after.json').read_text()))
if failure:
    sys.exit(failure)
PY
else
python3 -c 'import socket,sys,time; host=sys.argv[1]; end=time.monotonic()+90
while time.monotonic()<end:
    try:
        s=socket.create_connection((host,4299),2)
        s.sendall(b"PING"+bytes(12)); s.close()
        print("Homebrew Channel is ready for another run")
        break
    except OSError:
        time.sleep(1)
else:
    sys.exit("Homebrew Channel did not return within 90 seconds; check the Wii screen")' "$wii_ip"
fi
python3 scripts/check_hardware_run.py "$out"
if [ "$chain" = smoke ]; then python3 scripts/hardware_smoke_check.py "$out"; fi
case "$chain" in
mario_kart_warm|mario_kart_race|mario_titles_warm) python3 scripts/mario_kart_check.py "$out" ;;
esac
echo "Hardware run complete: $out"
