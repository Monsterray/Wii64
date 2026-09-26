#!/usr/bin/env bash
# stage_roms.sh <profile dir> -- copy new or changed ROMs (and scripts/inputs/ pad replays) from the one drop folder into a
# Dolphin profile's SD sync folder. Every launcher (dolphin_test.sh, wii64_diag.sh start and
# chain) calls this just before Dolphin starts, so a ROM added to the drop folder shows up
# on the next launch of any profile.
#
# Why copies: Dolphin's sync folder is fixed at <userdir>/Load/WiiSDSync and its folder
# sync does not follow junctions (a junctioned roms/ syncs as empty). Dolphin packs the
# folder into WiiSD.raw at boot and writes the image back over the folder at close, so:
#   - a running Dolphin never sees a ROM added after it booted -- restart it;
#   - never add ROMs straight into a profile's WiiSDSync while its Dolphin runs -- the
#     sync-back at close deletes them. Add them to the drop folder only.
set -euo pipefail
case "$(uname -s)" in
	Darwin) DEFAULT_ROM_DIR="$HOME/Library/Application Support/Dolphin/Load/WiiSDSync/wii64/roms" ;;
	*) DEFAULT_ROM_DIR="/c/tools/Dolphin-x64/User/Load/WiiSDSync/wii64/roms" ;;
esac
SHARED_ROMS="${WII64_ROM_DIR:-$DEFAULT_ROM_DIR}"
dest="${1:?usage: stage_roms.sh <profile dir>}/Load/WiiSDSync/wii64"
mkdir -p "$dest/roms" "$dest/saves"

# boxart.bin (tracked, release/wii64/) belongs in wii64/, not roms/ -- without it every
# game shows "boxart.bin Missing".
cp -u "$(dirname "${BASH_SOURCE[0]}")/../release/wii64/boxart.bin" "$dest/boxart.bin" 2>/dev/null || true

[ -d "$SHARED_ROMS" ] || { echo "stage_roms: no drop folder $SHARED_ROMS" >&2; exit 0; }
# -iname: a plain *.v64 glob misses "Banjo-Kazooie.V64". Extensions match
# fileBrowser-libfat.c's n64only whitelist.
n=0
# Scan nested drop-folder groups too; the Wii library itself is flat, so keep
# each file's basename when staging it.
while IFS= read -r -d '' f; do
	t="$dest/roms/$(basename "$f")"
	if [ ! -e "$t" ] || [ "$f" -nt "$t" ]; then cp -p "$f" "$t"; echo "staged: $(basename "$f")"; n=$((n + 1)); fi
done < <(find "$SHARED_ROMS" -type f \
	\( -iname '*.z64' -o -iname '*.n64' -o -iname '*.v64' -o -iname '*.bin' -o -iname '*.rom' \) -print0)
# Pad replays (chain=<vis>,input=<name>, scripts/dtm2input.py) go to wii64/input/.
mkdir -p "$dest/input"
cp -u "$(dirname "${BASH_SOURCE[0]}")"/../scripts/inputs/*.txt "$dest/input/" 2>/dev/null || true
echo "stage_roms: $n new, $(ls "$dest/roms" | wc -l) ROMs in $(basename "$(dirname "$(dirname "$(dirname "$dest")")")")"
