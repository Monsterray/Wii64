import contextlib
import io
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from subsystem_report import load, milliseconds, report
from subsystem_compare import compare

log = """subsystem_time: stage=lookup calls=254 timed_calls=2 period=127 timed_us=200
subsystem_time: stage=limiter calls=100 timed_calls=100 period=1 timed_us=1000000
game: n=1/1 how=vis vis=900 vi_rate=60 wall_us=15000000 sleep_us=900000 pmc1=100 pmc2=90 underruns=1 overruns=0 rom=sd:/wii64/roms/Test.z64
game: n=2/2 how=vis vis=900 vi_rate=50 wall_us=18000000 pmc1=100 pmc2=90 rom=sd:/wii64/roms/Next.z64
"""
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "perf.log"
    path.write_text(log.replace("subsystem_time:", "subsystem_time:\0"))
    rows = load(path)
assert milliseconds(rows[0]["subsystems"]["lookup"]) == 25.4
assert milliseconds({"calls": 100, "timed_calls": 0}) is None
assert not rows[1]["subsystems"]
out = io.StringIO()
with contextlib.redirect_stdout(out):
    report(rows)
assert "+100.0 ms" in out.getvalue() and "1.000x" in out.getvalue()
assert "Do not sum" in out.getvalue() and "probes absent" in out.getvalue()
alias = dict(rows[0], subsystems={"execute_inclusive": {
    "calls": 12700, "timed_calls": 100, "period": 127, "timed_us": 200000}})
out = io.StringIO()
with contextlib.redirect_stdout(out):
    report([alias])
assert "estimate" in out.getvalue() and "sampling estimate exceeds wall time" in out.getvalue()
assert milliseconds(alias["subsystems"]["execute_inclusive"]) > alias["wall_us"] / 1000
control = dict(rows[0], subsystems={})
result = compare(rows[:1], [control], rows[:1])
assert result[0]["cycles_percent"] == 0
for invalid in ([rows[1]], [], [dict(rows[0], pmc2=0)]):
    try:
        compare(rows[:1], invalid, rows[:1])
    except ValueError:
        pass
    else:
        raise AssertionError("mismatched/host/incomplete run accepted")
print("subsystem reports, optional probes, inclusive estimates and A/B/A checks: ok")
