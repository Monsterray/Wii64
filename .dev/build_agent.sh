#!/usr/bin/env bash
# Build HBC-Reborn's agent library with Wii64's compiler and libogc2, into Wii64's own
# .dev/hbc_agent/ -- not into the shared HBC-Reborn checkout, where another project's
# toolchain (WiiStation's r41-2) would overwrite it. Makefile.wii links that copy.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh
hbc_root="${WII64_HBC_ROOT:-$REPO_ROOT/../hbc-reborn}"
[[ -f "$hbc_root/sdk/hbc_agent/Makefile" ]] || {
    echo "Clone hbc-reborn beside Wii64, or set WII64_HBC_ROOT to its checkout." >&2; exit 2;
}
# The same libogc2 Makefile.wii picks: the Windows side-by-side SDK first.
libogc2="${LIBOGC2:-$DEVKITPRO/wii64-sdk/libogc2}"
[ -d "$libogc2" ] || libogc2="$DEVKITPRO/libogc2"
ogc_include="$libogc2/include"
if [ ! -d "$ogc_include" ]; then ogc_include="$libogc2/wii/include"; fi
[[ -d "$ogc_include" ]] || { echo "Install libogc2 first." >&2; exit 2; }
echo "Building HBC agent with $DEVKITPPC and $ogc_include"
compat=""
# libogc2 r1 renamed C fields without changing its assembly frame. HBC-Reborn after
# 1.9.3 handles it; for an older checkout, map the names. Keep the SDK's offset
# assertions active; probe the installed headers, not a version pin.
if ! grep -q FC_SRR0 "$hbc_root/sdk/hbc_agent/agent.c" &&
    printf '#include <ogc/context.h>\nint check(void) { return ((frame_context*)0)->srr0; }\n' | \
    "$DEVKITPPC/bin/powerpc-eabi-gcc" -I"$ogc_include" -x c -fsyntax-only - 2>/dev/null; then
    compat='-DEXCPT_Number=nExcept -DSRR0=srr0 -DSRR1=srr1 -DGPR=gpr -DGQR=gqr -DCR=cr -DLR=lr -DCTR=ctr -DXER=xer'
fi
if ! printf '#include <ogc/message.h>\nint check(void) { return MQ_ERROR_SUCCESSFUL; }\n' | \
    "$DEVKITPPC/bin/powerpc-eabi-gcc" -I"$ogc_include" -x c -fsyntax-only - 2>/dev/null; then
    compat+=' -DMQ_ERROR_SUCCESSFUL=0'
fi
out="$REPO_ROOT/.dev/hbc_agent"
# The SDK's make targets do not escape spaces in OUT/BUILD. Keep its generated
# files in a space-free directory, then publish the archive at the usual path.
task_agent_build="$(mktemp -d /tmp/wii64-hbc-agent.XXXXXX)"
trap 'rm -rf -- "$task_agent_build"' EXIT
# SDK objects have no compiler/flag fingerprint: rebuild its small archive.
# The SDK makefile adds -I$(DEVKITPRO)/$(OGC)/include: name this libogc2's own include
# directory that way, so no other libogc2 (WiiStation's) is ever on the path.
ogc_name="${ogc_include#"$(dirname "$libogc2")"/}"
make -B -C "$hbc_root/sdk/hbc_agent" -j4 OGC="${ogc_name%/include}" \
    DEVKITPRO="$(dirname "$libogc2")" DEVKITPPC="$DEVKITPPC" TMP="$WII64_BUILD_TMP" TEMP="$WII64_BUILD_TMP" \
    OUT="$task_agent_build/libhbcagent.a" BUILD="$task_agent_build/build" \
    "EXTRA_CFLAGS=-I\"$ogc_include\" -I\"$DEVKITPRO/portlibs/ppc/include\" $compat"
mkdir -p "$out"
cp "$task_agent_build/libhbcagent.a" "$out/libhbcagent.a"
echo "Agent library: $out/libhbcagent.a ($(git -C "$hbc_root" rev-parse --short=7 HEAD))."
echo "Every Wii build links it; clean Wii64 and rebuild (.dev/build.sh <target>) to pick up a new one."
