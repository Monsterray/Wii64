"""Run with python3 tests/dolphin_log_test.py; no Dolphin instance is needed."""
import gzip
import os
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "scripts"))
from dolphin_log import latest_tail, within_budget

with tempfile.TemporaryDirectory(prefix="wii64-log-") as directory:
    path = Path(directory) / "Logs" / "dolphin.log"
    path.parent.mkdir()
    assert within_budget(path, 1)
    path.write_text("old fault\nStarting core = Wii mode\n" + "warning\n" * 150000 +
                    "Starting core = Wii mode\nnew boot\n")
    assert not within_budget(path, 1)
    assert within_budget(path, 2)
    assert latest_tail(path) == "Starting core = Wii mode\nnew boot\n"
    env = dict(os.environ, WII64_DOLPHIN_PROFILE=directory, WII64_DOLPHIN_MAX_LOG_MIB="1")
    result = subprocess.run(["bash", str(root / ".dev/dolphin_test.sh"), "missing.dol", "1"],
                            env=env, capture_output=True, text=True)
    assert result.returncode == 1 and "gzip its old log" in result.stderr
    env["WII64_DOLPHIN_MAX_LOG_MIB"] = "0"
    result = subprocess.run(["bash", str(root / ".dev/dolphin_test.sh"), "missing.dol", "1"],
                            env=env, capture_output=True, text=True)
    assert result.returncode == 2 and "must be positive" in result.stderr
    path.write_text("".join(f"line {i}\n" for i in range(100)))
    assert latest_tail(path, 3) == "line 97\nline 98\nline 99\n"
    archive = path.with_suffix(".log.gz")
    with gzip.open(archive, "wb") as log:
        log.write(path.read_bytes())
    assert latest_tail(archive, 3) == latest_tail(path, 3)
print("Dolphin log budget and bounded latest-boot tail: ok")
