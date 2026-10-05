#!/usr/bin/env python3
"""Cache report rejects absent, malformed, duplicate and incomplete results."""
import importlib.util
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
spec = importlib.util.spec_from_file_location("report", ROOT / "scripts/cache_probe_report.py")
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "perf.log"
    good = ("mark: cache_gx_test: stale_words=8 fresh_errors=0 guard_errors=0\n"
            "cache_texture: mip_calls=1 levels=2 staged_bytes=128 packed_bytes=128 timed_calls=1 sampled_us=10\n"
            "cache_xfb: calls=127 checks=1 stale_checks=1\n"
            "game: n=1 vi=900 us=15000000 rom=sd:/wii64/roms/test.v64\n")
    path.write_text(good)
    rows = report.read(path)
    assert len(rows) == 1 and rows[0]["gx_test"]["fresh_errors"] == 0
    assert rows[0]["game"]["vi"] == 900
    for bad in ("", good.replace("levels=2", "levels=broken"),
                good.replace("levels=2 ", ""), good.replace("mip_calls=1", "mip_calls=-1"),
                good.replace("fresh_errors=0", "fresh_errors=1"),
                good.replace("packed_bytes=128", "packed_bytes=256"),
                good.replace("cache_xfb: calls=127 checks=1 stale_checks=1\n", ""),
                good.split("game:")[0], good.replace("cache_xfb:", "cache_texture:")):
        path.write_text(bad)
        try:
            report.read(path)
        except ValueError:
            pass
        else:
            raise AssertionError("incomplete/malformed probe result accepted")
print("cache probe report: complete rows and rejected malformed results PASS")
