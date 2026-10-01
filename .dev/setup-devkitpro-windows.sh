#!/usr/bin/env bash
# One-time setup of the libogc2 and libfat that Wii64 builds against on Windows.
#
# Wii64 needs current libogc2 (Extrems' fork) and a libfat built with the same
# devkitPPC. This script builds both from source checkouts with the current
# devkitPPC (pacman's $DEVKITPRO/devkitPPC), using their upstream steps:
#   libogc2: make, make install         libfat: make ogc-release, make ogc-install
# It installs them into their own devkitPro-shaped root, C:\devkitPro\wii64-sdk
# (so C:\devkitPro\wii64-sdk\libogc2\wii\{include,lib}), side by side with any
# other libogc2. C:\devkitPro\libogc2 -- the prebuilt r41-2 copy WiiStation uses --
# is never touched. .dev/env.sh points the build there.
#
# Prerequisites, once, by hand (the installer needs admin rights):
#   1. Install devkitPro with "Wii Development" checked, at C:\devkitPro:
#      https://github.com/devkitPro/installer/releases/latest
#   2. Git for Windows.
#
# Source checkouts live beside this repo (AGENTS.md: separate checkouts) and are
# cloned when missing. Override with LIBOGC2_SRC, LIBFAT_SRC or WII64_SDK.
# Safe to re-run: run it again after pulling either checkout.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEVKITPRO=/c/devkitPro
DEVKITPPC="$DEVKITPRO/devkitPPC"
SDK="${WII64_SDK:-$DEVKITPRO/wii64-sdk}"
LIBOGC2_SRC="${LIBOGC2_SRC:-$(dirname "$REPO_ROOT")/libogc2-src}"
LIBFAT_SRC="${LIBFAT_SRC:-$(dirname "$REPO_ROOT")/libfat-src}"
# The libogc2 and libfat this setup was last tested with. Newer ones usually work;
# a mismatch is reported, not refused.
TESTED_LIBOGC2=0866d8b
TESTED_LIBFAT=c756e1e

log() { printf '\n==> %s\n' "$1"; }
die() { printf 'ERROR: %s\n' "$1" >&2; exit 1; }

[ -x "$DEVKITPPC/bin/powerpc-eabi-gcc.exe" ] ||
	die "devkitPPC not found at $DEVKITPPC. Install devkitPro with \"Wii Development\" (see this script's header)."
[ -x "$DEVKITPRO/msys2/usr/bin/make.exe" ] ||
	die "devkitPro's MSYS2 make not found at $DEVKITPRO/msys2/usr/bin. Reinstall devkitPro with \"Wii Development\"."
export PATH="$DEVKITPRO/msys2/usr/bin:$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"
# The assembler and linker need native TMP/TEMP paths, and make here does not pass the
# environment on, so they go on every make command line (as in .dev/build.sh).
mkdir -p "$REPO_ROOT/.dev/build_tmp"
TMPW="$(cd "$REPO_ROOT/.dev/build_tmp" && pwd -W | tr '/' '\')"
mk() { make TMP="$TMPW" TEMP="$TMPW" DEVKITPPC="$DEVKITPPC" "$@"; }

checkout() { # <dir> <url> <tested commit>
	[ -d "$1/.git" ] || { log "Cloning $2 into $1"; git clone -q "$2" "$1"; }
	local head; head="$(git -C "$1" rev-parse --short=7 HEAD)"
	[ "$head" = "$3" ] || echo "note: $(basename "$1") is at $head; this setup was tested with $3."
}
checkout "$LIBOGC2_SRC" https://github.com/extremscorner/libogc2.git "$TESTED_LIBOGC2"
checkout "$LIBFAT_SRC" https://github.com/extremscorner/libfat.git "$TESTED_LIBFAT"

# Build with the real devkitPro root; install with DEVKITPRO pointed at the SDK
# root, which is where both upstream install rules put libogc2/.
log "Building libogc2 ($LIBOGC2_SRC)"
mk -C "$LIBOGC2_SRC" DEVKITPRO="$DEVKITPRO" -j"$(nproc)"
log "Installing libogc2 into $SDK/libogc2"
mk -C "$LIBOGC2_SRC" DEVKITPRO="$SDK" install >/dev/null

# libfat reads $(DEVKITPRO)/libogc2/wii_rules, so it builds against the SDK root.
# Its optional libogc-rice variant stops with "Please set DEVKITRICE" and
# "Error 2 (ignored)": upstream marks that variant optional; the libogc2 one is ours.
log "Building libfat ($LIBFAT_SRC)"
mk -C "$LIBFAT_SRC" DEVKITPRO="$SDK" ogc-release
log "Installing libfat into $SDK/libogc2"
mk -C "$LIBFAT_SRC" DEVKITPRO="$SDK" ogc-install >/dev/null

{
	echo "libogc2 $(git -C "$LIBOGC2_SRC" rev-parse --short=7 HEAD) $LIBOGC2_SRC"
	echo "libfat  $(git -C "$LIBFAT_SRC" rev-parse --short=7 HEAD) $LIBFAT_SRC"
	echo "devkitPPC $("$DEVKITPPC/bin/powerpc-eabi-gcc.exe" -dumpversion)"
} > "$SDK/VERSIONS"
for f in wii/include/ogc/video.h wii/include/fat.h wii/lib/libogc.a wii/lib/libfat.a; do
	[ -f "$SDK/libogc2/$f" ] || die "install incomplete: $SDK/libogc2/$f is missing."
done
log "Done. Installed:"
cat "$SDK/VERSIONS"
echo "Build with: .dev/build.sh glN64_wii"
