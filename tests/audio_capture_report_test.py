"""Small WAV fixtures for the Dolphin capture signal check."""

import pathlib
import subprocess
import sys
import tempfile
import wave


with tempfile.TemporaryDirectory() as folder:
    root = pathlib.Path(folder)
    for name, pcm in (("silent", b"\0\0" * 4), ("signal", b"\0\0\xff\x7f\0\x80\0\0")):
        with wave.open(str(root / f"{name}.wav"), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(48000)
            wav.writeframes(pcm)
    script = pathlib.Path(__file__).resolve().parents[1] / "scripts/audio_capture_report.py"
    output = subprocess.check_output([sys.executable, str(script), str(root)], text=True)
    assert "silent.wav" in output and "nonzero=0, peak=0, RMS=0.0" in output
    assert "signal.wav" in output and "nonzero=2, peak=32768" in output
print("audio capture report: ok")
