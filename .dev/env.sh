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
	export DEVKITPRO="${DEVKITPRO:-/c/devKitPro}"
	export DEVKITPPC="${DEVKITPPC:-$DEVKITPRO/devkitPPC-r41-2}"
	export PATH="$DEVKITPRO/tools/bin:$PATH"
	mkdir -p "$REPO_ROOT/.dev/build_tmp"
	export WII64_BUILD_TMP="$(cd "$REPO_ROOT/.dev/build_tmp" && pwd -W | tr '/' '\\')"
	;;
esac

mkdir -p "$WII64_BUILD_TMP"
