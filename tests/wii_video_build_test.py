"""Atomic compile/sign, content-aware cache, spaced paths and failure preservation."""
from pathlib import Path
import runpy
import subprocess
import tempfile
from unittest.mock import patch

build = runpy.run_path(str(Path(__file__).resolve().parents[1] /
                          "scripts/build_wii_video.py"))["build"]
with tempfile.TemporaryDirectory(prefix="wii64 video build ") as directory:
    root = Path(directory)
    (root / "scripts").mkdir()
    for name in ("wii_video.swift", "wii_video.plist", "build_wii_video.py"):
        (root / "scripts" / name).write_text("fixture: " + name)
    calls = []
    fail = [False]
    def invoke(command, **kwargs):
        calls.append(command)
        if fail[0]:
            raise subprocess.CalledProcessError(1, command)
        if command[0] == "swiftc":
            assert "__info_plist" in command
            assert str(root / "scripts/wii_video.plist") in command
            Path(command[-1]).write_bytes(b"compiled native helper")
    with patch.dict(build.__globals__, ROOT=root), patch("sys.platform", "darwin"), \
         patch("subprocess.run", side_effect=invoke):
        binary = build()
        previous = binary.read_bytes()
        assert len(calls) == 3
        build()
        assert len(calls) == 3, "Recompiled unchanged inputs"
        # A modified binary is not trusted merely because its source mtime matches.
        binary.write_bytes(b"truncated")
        build()
        assert len(calls) == 6 and binary.read_bytes() == previous
        (root / "scripts/wii_video.swift").write_text("changed source")
        fail[0] = True
        try:
            build()
        except subprocess.CalledProcessError:
            pass
        else:
            raise AssertionError("Accepted compile failure")
        assert binary.read_bytes() == previous
print("Capture build: cached content, signature checks and preserved previous binary PASS")
