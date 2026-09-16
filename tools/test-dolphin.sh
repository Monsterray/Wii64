#!/usr/bin/env bash
set -eu

case_name=${1:?usage: make test-dolphin CASE=mario-kart-64}
root=$(cd "$(dirname "$0")/.." && pwd)
case_file="$root/tests/dolphin/cases/$case_name.ini"
sd_root=${WII64_DOLPHIN_SD_ROOT:-"$HOME/Library/Application Support/Dolphin/Load/WiiSDSync"}
dolphin=${DOLPHIN_BIN:-/Applications/Dolphin.app/Contents/MacOS/Dolphin}
timeout=${WII64_TEST_TIMEOUT:-120}
jobs=${WII64_TEST_JOBS:-$(sysctl -n hw.ncpu)}

: "${DEVKITPRO:=/opt/devkitpro}"
: "${DEVKITPPC:=$DEVKITPRO/devkitPPC}"
export DEVKITPRO DEVKITPPC
export PATH="$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"

test -f "$case_file"
test -x "$dolphin"
if pgrep -x Dolphin >/dev/null 2>&1; then
  printf 'Close Dolphin before running an automated test.\n' >&2
  exit 2
fi

rom=$(awk -F= '$1 == "rom" { print substr($0, 5); exit }' "$case_file")
vis=$(awk -F= '$1 == "vis" { print $2; exit }' "$case_file")
test -n "$rom"
test -n "$vis"
test -f "$sd_root/wii64/roms/$rom"

artifacts=$(mktemp -d "/private/tmp/wii64-dolphin-test.$case_name.XXXXXX")
control_dir="$sd_root/wii64/autotest"
result_dir="$control_dir/results"
control="$control_dir/current.ini"
result="$result_dir/$case_name.result"
state="$control_dir/state.txt"
mkdir -p "$result_dir"
rm -f "$result"
rm -f "$state"
cleanup() {
  make -C "$root" -f Makefile.glN64_wii clean >/dev/null
}
trap cleanup EXIT
printf 'rom=sd:/wii64/roms/%s\nvis=%s\nresult=sd:/wii64/autotest/results/%s.result\n' \
  "$rom" "$vis" "$case_name" > "$control"

make -C "$root" -f Makefile.glN64_wii clean
make -C "$root" -f Makefile.glN64_wii -j"$jobs" AUTOTEST=1 \
  ELF="$artifacts/wii64-glN64-autotest.elf" DOL="$artifacts/wii64-glN64-autotest.dol"

nohup "$dolphin" --batch --audio_emulation LLE \
  --config Main.Core.MMU=True \
  --config Main.Core.WiiSDCardEnableFolderSync=True \
  --config "Main.General.WiiSDCardSyncFolder=$sd_root" \
  --exec="$artifacts/wii64-glN64-autotest.dol" > "$artifacts/dolphin.log" 2>&1 &
pid=$!
stop_dolphin() {
  kill -KILL "$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
}
deadline=$(( $(date +%s) + timeout ))
while ! test -f "$result" && test "$(date +%s)" -lt "$deadline"; do
  sleep 1
done

if test -f "$result" && grep -qx 'status=pass' "$result"; then
  cp "$result" "$artifacts/result.txt"
  test ! -f "$state" || cp "$state" "$artifacts/state.txt"
  stop_dolphin
  printf 'PASS %s\nArtifacts: %s\n' "$case_name" "$artifacts"
else
  test ! -f "$state" || cp "$state" "$artifacts/state.txt"
  stop_dolphin
  printf 'FAIL %s: no result after %ss\nArtifacts: %s\n' "$case_name" "$timeout" "$artifacts" >&2
  exit 1
fi
