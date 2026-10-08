"""Exercise the SDK launcher's output paths with a spaced checkout; no Wii access."""
from pathlib import Path
import os
import shlex
import shutil
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="wii64-agent-test-") as directory:
    root = Path(directory) / "spaced checkout"
    (root / ".dev").mkdir(parents=True)
    shutil.copy2(source / ".dev/build_agent.sh", root / ".dev/build_agent.sh")
    (root / ".dev/env.sh").write_text('''REPO_ROOT="$(pwd)"
export DEVKITPRO="$REPO_ROOT/toolchain"
export DEVKITPPC="$DEVKITPRO/devkitPPC"
export WII64_BUILD_TMP="$REPO_ROOT/.dev/tmp"
''')
    sdk = root / "hbc source/sdk/hbc_agent"
    sdk.mkdir(parents=True)
    # The current SDK chooses exception field names from libogc2's version
    # macro, which context.h does not include. Exercise that actual compile seam.
    (sdk / "agent.c").write_text('''#include <stddef.h>
#include <ogc/context.h>
#ifdef _LIBOGC2_REVISION_
#define FC_SRR0 srr0
#else
#define FC_SRR0 SRR0
#endif
_Static_assert(offsetof(frame_context, FC_SRR0) == 0, "exception layout");
''')
    include = root / "toolchain/libogc2/include/ogc"
    include.mkdir(parents=True)
    (include / "context.h").write_text("typedef struct { unsigned srr0; } frame_context;\n")
    (include / "libversion.h").write_text("#define _LIBOGC2_REVISION_ 2464\n")
    compiler = root / "toolchain/devkitPPC/bin/powerpc-eabi-gcc"
    compiler.parent.mkdir(parents=True)
    compiler.write_text('#!/bin/sh\nexec ' + shlex.quote(shutil.which("cc")) + ' "$@"\n')
    compiler.chmod(0o755)
    # Same OUT/BUILD target syntax as the actual SDK, not a fake make command.
    (sdk / "Makefile").write_text('''$(OUT): $(BUILD)/fixture.o
	printf 'fixture archive' > $@
$(BUILD)/fixture.o:
	mkdir -p $(BUILD)
	"$(DEVKITPPC)/bin/powerpc-eabi-gcc" $(EXTRA_CFLAGS) -c agent.c -o $@
''')
    env = dict(os.environ, WII64_HBC_ROOT=str(sdk.parents[1]))
    result = subprocess.run(["bash", ".dev/build_agent.sh"], cwd=root, env=env,
                            text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr
    assert (root / ".dev/hbc_agent/libhbcagent.a").read_text() == "fixture archive"
print("HBC SDK output paths: spaced checkout and SDK path passed")
