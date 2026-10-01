#!/usr/bin/env bash
# Local toolchain locations for building/testing Wii64.
#
# Source this, don't execute it: `source .dev/env.sh`

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

case "$(uname -s)" in
	Darwin)
	export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
	export DEVKITPPC="${DEVKITPPC:-$DEVKITPRO/devkitPPC}"
	export PATH="$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"
	export WII64_BUILD_TMP="$REPO_ROOT/.dev/build_tmp"
	;;
	*)
	# Forced, not ${VAR:-}: shells here often carry DEVKITPRO=/opt/devkitpro, which
	# only devkitPro's own MSYS2 understands, and WiiStation's r41-2 pin must not
	# leak in. Wii64 uses pacman's current devkitPPC with the libogc2 and libfat
	# that .dev/setup-devkitpro-windows.sh installs into C:\devkitPro\wii64-sdk.
	export DEVKITPRO=/c/devkitPro
	export DEVKITPPC="$DEVKITPRO/devkitPPC"
	export PATH="$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"
	# On Windows python3 is often only the Microsoft Store placeholder; python.org's
	# installer provides python. The scripts call python3.
	if ! python3 -c '' >/dev/null 2>&1 && command -v python >/dev/null; then
		python3() { python "$@"; }
		export -f python3
	fi
	mkdir -p "$REPO_ROOT/.dev/build_tmp"
	export WII64_BUILD_TMP="$(cd "$REPO_ROOT/.dev/build_tmp" && pwd -W | tr '/' '\\')"
	;;
esac

mkdir -p "$WII64_BUILD_TMP"
