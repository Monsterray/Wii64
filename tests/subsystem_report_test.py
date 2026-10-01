import contextlib
import io
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from subsystem_report import load, milliseconds, report
from subsystem_compare import compare

log = """mark: ROMCache_load: enter
mark: ROMCache_load: preflush_us=15000000 writes=256 bytes=16711680
mark: ROMCache_load: elapsed_us=31000000 bytes=33554432 vm=1
vm_io: read_ahead=1 reads=7 hits=21 read_bytes=229376 writes=0 write_bytes=0 errors=0
subsystem_time: stage=lookup calls=254 timed_calls=2 period=127 timed_us=200
subsystem_time: stage=limiter calls=100 timed_calls=100 period=1 timed_us=1000000
audio_time: stage=resample sampled_calls=7 sampled_us=100
game: n=1/1 how=vis vis=900 vi_rate=60 wall_us=15000000 sleep_us=900000 pmc1=100 pmc2=90 underruns=1 overruns=0 rom=sd:/wii64/roms/Test.z64
game: n=2/2 how=vis vis=900 vi_rate=50 wall_us=18000000 pmc1=100 pmc2=90 rom=sd:/wii64/roms/Next.z64
"""
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "perf.log"
    path.write_text(log.replace("subsystem_time:", "subsystem_time:\0"))
    rows = load(path)
    path.write_text(log.replace("calls=254 ", ""))
    try:
        load(path)
    except ValueError as error:
        assert "Incomplete subsystem timer" in str(error)
    else:
        raise AssertionError("incomplete timer accepted")
assert milliseconds(rows[0]["subsystems"]["lookup"]) == 25.4
assert milliseconds({"calls": 100, "timed_calls": 0}) is None
assert not rows[1]["subsystems"]
assert rows[0]["vm_io"]["hits"] == 21 and not rows[1]["vm_io"]
assert rows[0]["startup"]["elapsed_us"] == 31000000 and not rows[1]["startup"]
assert rows[0]["startup"]["preflush"]["writes"] == 256
assert rows[0]["audio_stages"]["resample"]["sampled_us"] == 100 and not rows[1]["audio_stages"]
out = io.StringIO()
with contextlib.redirect_stdout(out):
    report(rows)
assert "+100.0 ms" in out.getvalue() and "1.000x" in out.getvalue()
assert "ROM load 31.000 s" in out.getvalue() and "Preflush: 15.000 s" in out.getvalue()
assert "Do not sum" in out.getvalue() and "probes absent" in out.getvalue()
assert "absent=audio_submit,audio_callback" in out.getvalue()
assert "Not directly measured: GPU execution" in out.getvalue()
assert "sampled only" in out.getvalue()
out = io.StringIO()
with contextlib.redirect_stdout(out):
    report([dict(rows[0], how="load_failed", vis=0, wall_us=0),
            dict(rows[0], subsystems={"rsp_gfx": {"calls": 3, "timed_calls": 3, "timed_us": 2000}})])
assert "timers excluded" in out.getvalue() and "Scene warning" in out.getvalue()
assert "limiter" not in out.getvalue().split("Scene warning")[0]
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
no_work = [dict(rows[0], sleep_us=rows[0]["wall_us"])]
assert compare(no_work, [dict(no_work[0], subsystems={})], no_work)[0]["non_sleep_percent"] is None
assert compare(rows[:1], rows[:1], rows[:1], same_probes=True)[0]["wall_percent"] == 0
try:
    compare(rows[:1], [dict(rows[0], vm_io={"errors": 1})], rows[:1], same_probes=True)
except ValueError:
    pass
else:
    raise AssertionError("failed NAND I/O accepted for performance comparison")
for invalid in ([rows[1]], [], [dict(rows[0], pmc2=0)], [dict(rows[0], vm_io={"errors": 1})]):
    try:
        compare(rows[:1], invalid, rows[:1])
    except ValueError:
        pass
    else:
        raise AssertionError("mismatched/host/incomplete run accepted")
try:
    zero = [dict(rows[0], vis=0)]
    compare(zero, [dict(zero[0], subsystems={})], zero)
except ValueError:
    pass
else:
    raise AssertionError("zero-VI successful label accepted")
print("subsystem reports, optional probes, inclusive estimates and A/B/A checks: ok")
