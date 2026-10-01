#!/usr/bin/env bash
# Build the external SDK with Wii64's installed compiler and libogc2.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
source .dev/env.sh
hbc_root="${WII64_HBC_ROOT:-$REPO_ROOT/../hbc-reborn}"
[[ -f "$hbc_root/sdk/hbc_agent/Makefile" ]] || {
    echo "Clone hbc-reborn beside Wii64, or set WII64_HBC_ROOT to its checkout." >&2; exit 2;
}
ogc_include="$DEVKITPRO/libogc2/include"
if [ ! -d "$ogc_include" ]; then ogc_include="$DEVKITPRO/libogc2/wii/include"; fi
[[ -d "$ogc_include" ]] || { echo "Install libogc2 first." >&2; exit 2; }
echo "Building HBC agent with $DEVKITPPC and $ogc_include"
compat=""
# libogc2 renamed C fields without changing its assembly frame. Keep the
# SDK's offset assertions active; probe the installed headers, not a version pin.
if printf '#include <ogc/context.h>\nint check(void) { return ((frame_context*)0)->srr0; }\n' | \
    "$DEVKITPPC/bin/powerpc-eabi-gcc" -I"$ogc_include" -x c -fsyntax-only - 2>/dev/null; then
    compat='-DEXCPT_Number=nExcept -DSRR0=srr0 -DSRR1=srr1 -DGPR=gpr -DGQR=gqr -DCR=cr -DLR=lr -DCTR=ctr -DXER=xer'
fi
if ! printf '#include <ogc/message.h>\nint check(void) { return MQ_ERROR_SUCCESSFUL; }\n' | \
    "$DEVKITPPC/bin/powerpc-eabi-gcc" -I"$ogc_include" -x c -fsyntax-only - 2>/dev/null; then
    compat+=' -DMQ_ERROR_SUCCESSFUL=0'
fi
# SDK objects have no compiler/flag fingerprint: rebuild its small archive.
make -B -C "$hbc_root/sdk/hbc_agent" -j4 OGC=libogc2 \
    "EXTRA_CFLAGS=-I\"$ogc_include\" $compat"
echo "SDK ready. Clean Wii64, then build with HBC_AGENT=1 HBC_AGENT_ROOT=$hbc_root"
