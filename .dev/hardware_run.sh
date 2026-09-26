#!/usr/bin/env bash
# Build, launch through Homebrew Channel, and collect one Wii hardware run.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh

config=.dev/hardware.env
[ -f "$config" ] || { echo "Run .dev/hardware_setup.sh first." >&2; exit 1; }
value() { sed -n "s/^$1=//p" "$config" | tail -n 1; }
wii_ip="$(value WII64_WII_IP)"
mac_ip="$(value WII64_MAC_IP)"
python3 -c 'import ipaddress,sys; [ipaddress.IPv4Address(x) for x in sys.argv[1:]]' "$wii_ip" "$mac_ip"
python3 -c 'import socket,sys; s=socket.socket(); s.bind((sys.argv[1],0)); s.close(); socket.create_connection((sys.argv[2],4299),3).close()' "$mac_ip" "$wii_ip" || { echo "Homebrew Channel is not reachable; check the Wii screen and LAN." >&2; exit 1; }
chain="${2:-$(value WII64_CHAIN)}"
[[ "$chain" =~ ^[A-Za-z0-9_-]+$ && -f "scripts/chains/$chain.txt" ]] || {
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
	.dev/build_profiling.sh "$target"
fi
[[ -f "$dol" ]] || { echo "Missing $dol; build it or unset WII64_SKIP_BUILD=1." >&2; exit 1; }
mkdir -p .dev/runs
out="$(mktemp -d ".dev/runs/hardware-${target}-$(date +%Y%m%d-%H%M%S)-XXXX")"
cp "scripts/chains/$chain.txt" "$out/diag.cfg"
printf 'result_host=%s\n' "$mac_ip" >> "$out/diag.cfg"
receiver_python=python3
if [ "$(uname -s)" = Darwin ] && [ -x /usr/bin/python3 ]; then receiver_python=/usr/bin/python3; fi
receiver_timeout=1200
if [ "$chain" = hardware ]; then receiver_timeout=2400; fi
"$receiver_python" scripts/hardware_receive.py "$out" --bind "$mac_ip" --wii-ip "$wii_ip" --timeout "$receiver_timeout" >"$out/receiver.log" 2>&1 &
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
	diag_args+=("--diag=$line")
done < "$out/diag.cfg"
((${#diag_args[@]} <= 32)) || { echo "The Wii accepts at most 32 diagnostic lines." >&2; exit 1; }
WIILOAD="tcp:$wii_ip" wiiload "$dol" "${diag_args[@]}"
echo "Waiting for the Wii to finish; results will arrive in $out"
wait "$receiver"
trap - EXIT
python3 scripts/chain_table.py "$out" | tee "$out/summary.txt"
echo "Waiting for Homebrew Channel to return..."
python3 -c 'import socket,sys,time; host=sys.argv[1]; end=time.monotonic()+90
while time.monotonic()<end:
    try:
        socket.create_connection((host,4299),2).close()
        print("Homebrew Channel is ready for another run")
        break
    except OSError:
        time.sleep(1)
else:
    sys.exit("Homebrew Channel did not return within 90 seconds; check the Wii screen")' "$wii_ip"
python3 scripts/check_hardware_run.py "$out"
if [ "$chain" = smoke ]; then python3 scripts/hardware_smoke_check.py "$out"; fi
echo "Hardware run complete: $out"
