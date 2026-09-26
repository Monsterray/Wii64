#!/usr/bin/env bash
# Build a Wii64 target with the host's configured devkitPPC toolchain.
# See README.md's "BUILDING FROM SOURCE".
#
# Usage: .dev/build.sh <target> [make args...]
#   e.g. .dev/build.sh glN64_wii
#        .dev/build.sh Rice_wii clean
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh

target="${1:?usage: .dev/build.sh <target e.g. glN64_wii|Rice_wii|glN64_gc|Rice_gc> [make args...]}"
shift || true

# PATH already includes $DEVKITPRO/tools/bin (elf2dol, gxtexconv, ...), so the
# default goal (the .dol) builds all the way through in one step.
make -f "Makefile.${target}" \
	DEVKITPPC="$DEVKITPPC" DEVKITPRO="$DEVKITPRO" \
	TMP="$WII64_BUILD_TMP" TEMP="$WII64_BUILD_TMP" \
	-j4 "$@"
