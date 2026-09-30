"""Run with: python3 tests/audio_report_test.py"""
import contextlib
import io
import os
import sys
from unittest.mock import patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))
from audio_report import report


log = """cpu: queue_ms=0
cpu: queue_ms=100
audio_gaps: nead_mats=1 nead_efz=0 resample_flag2=0 musyx_ptr10=2
audio_output: stream_requests=50 stream_fed=48 input_hz=32000 queue_peak_ms=120
game: n=1/1 how=vis vis=900 underruns=0 pmc1=123 rom=sd:/wii64/roms/Test.z64
cpu: queue_ms=20
game: n=1/1 how=vis vis=900 underruns=0 pmc1=456 rom=sd:/wii64/roms/Next.z64
"""
output = io.StringIO()
with patch("builtins.open", return_value=io.StringIO(log)):
    with contextlib.redirect_stdout(output):
        report("unused")
result = output.getvalue()
assert "mean 50.0 ms, p95 100 ms (2 samples" in result
assert "unsupported    nead_mats=1, musyx_ptr10=2" in result
assert "AESND stream   48/50 buffers fed, input 32000 Hz, producer queue peak 120 ms" in result
assert "mean 20.0 ms, p95 20 ms (1 sample" in result
assert result.count("unsupported") == 1
print("audio report: ok")
