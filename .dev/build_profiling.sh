#!/usr/bin/env bash
# Build a Wii64 target with PERF_PROF compiled in (main/perf_prof.c's
# pageBegin/tileLoaded/pageEnd/mark calls become real instead of no-ops).
# The scripts are shared; build products under .dev/ stay local.
#
# -DPERF_PROF has to come from here, not a source edit: Makefile.base's
# DEBUG_FLAGS has it commented out, and there's no header-dependency
# tracking in this build (see doc/subsystem-review.md's Build System
# section), so a flag-only change needs a `clean` first or stale .o files
# silently keep the old defines.
#
# Usage: .dev/build_profiling.sh <target e.g. Rice_wii|glN64_wii> [make args...]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh

target="${1:?usage: .dev/build_profiling.sh <target> [make args...]}"
shift || true

make -f "Makefile.${target}" \
	DEVKITPPC="$DEVKITPPC" DEVKITPRO="$DEVKITPRO" \
	TMP="$WII64_BUILD_TMP" TEMP="$WII64_BUILD_TMP" \
	clean

make -f "Makefile.${target}" \
	DEVKITPPC="$DEVKITPPC" DEVKITPRO="$DEVKITPRO" \
	TMP="$WII64_BUILD_TMP" TEMP="$WII64_BUILD_TMP" \
	-j4 DEBUG_FLAGS=-DPERF_PROF "$@"
