"""Exercise the SDK launcher's output paths with a spaced checkout; no Wii access."""
from pathlib import Path
import os
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
    (sdk / "agent.c").write_text("/* FC_SRR0 */\n")
    (root / "toolchain/libogc2/include").mkdir(parents=True)
    # Same OUT/BUILD target syntax as the actual SDK, not a fake make command.
    (sdk / "Makefile").write_text('''$(OUT): $(BUILD)/fixture.o
	printf 'fixture archive' > $@
$(BUILD)/fixture.o:
	mkdir -p $(BUILD)
	touch $@
''')
    env = dict(os.environ, WII64_HBC_ROOT=str(sdk.parents[1]))
    result = subprocess.run(["bash", ".dev/build_agent.sh"], cwd=root, env=env,
                            text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr
    assert (root / ".dev/hbc_agent/libhbcagent.a").read_text() == "fixture archive"
print("HBC SDK output paths: spaced checkout and SDK path passed")
