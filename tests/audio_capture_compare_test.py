"""Synthetic checks for aligned Dolphin PCM capture comparison."""

import pathlib
import subprocess
import sys
import tempfile
import wave


script = pathlib.Path(__file__).resolve().parents[1] / "scripts/audio_capture_compare.py"


def write_wav(path, samples, channels=1, rate=48000):
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(2)
        wav.setframerate(rate)
        wav.writeframes(b"".join(int(value).to_bytes(2, "little", signed=True) for value in samples))


with tempfile.TemporaryDirectory() as folder:
    root = pathlib.Path(folder)
    silence = root / "silence.wav"
    silence_b = root / "silence-b.wav"
    write_wav(silence, [0] * 32)
    write_wav(silence_b, [0] * 40)
    assert "compared=0 frames" in subprocess.check_output(
        [sys.executable, str(script), str(silence), str(silence_b)], text=True)
    silent_interval = subprocess.run(
        [sys.executable, str(script), str(silence), str(silence_b),
         "--start-s", "0", "--duration-s", "0.0001"], text=True, capture_output=True)
    assert silent_interval.returncode != 0 and "non-silent" in silent_interval.stderr

    signal = [((i * 7919) % 30001) - 15000 for i in range(512)]
    shifted = root / "shifted.wav"
    identical = root / "identical.wav"
    changed = root / "changed.wav"
    write_wav(identical, [0] * 20 + signal + [0] * 20)
    write_wav(shifted, [0] * 37 + signal + [0] * 3)
    write_wav(changed, [0] * 37 + [v + (100 if i % 2 else -100)
                                   for i, v in enumerate(signal)] + [0] * 3)
    same = subprocess.check_output([sys.executable, str(script), str(identical), str(shifted)], text=True)
    diff = subprocess.check_output([sys.executable, str(script), str(identical), str(changed)], text=True)
    assert "lag=17 frames" in same and "sample-difference RMS=0.00" in same
    assert "correlation=1.000" in same
    assert "sample-difference RMS=100.00" in diff and "peak=100" in diff
    interval = subprocess.check_output(
        [sys.executable, str(script), str(identical), str(shifted),
         "--start-s", "0.003", "--duration-s", "0.005"], text=True)
    assert "lag=17 frames" in interval and "compared=240 frames" in interval
    assert "sample-difference RMS=0.00" in interval
    fixed = subprocess.check_output(
        [sys.executable, str(script), str(identical), str(shifted),
         "--start-s", "0.003", "--duration-s", "0.005", "--lag-frames", "17"], text=True)
    assert "lag=17 frames" in fixed and "sample-difference RMS=0.00" in fixed
    outside = subprocess.run(
        [sys.executable, str(script), str(identical), str(shifted),
         "--start-s", "1", "--duration-s", "1"], text=True, capture_output=True)
    assert outside.returncode != 0 and "outside the first WAV" in outside.stderr

    stereo_a = root / "stereo-a.wav"
    stereo_b = root / "stereo-b.wav"
    stereo_rate = root / "stereo-rate.wav"
    stereo_signal = [value for sample in signal for value in (sample, -sample)]
    write_wav(stereo_a, [0] * 40 + stereo_signal, channels=2)
    write_wav(stereo_b, [0] * 74 + stereo_signal, channels=2)
    write_wav(stereo_rate, [0] * 74 + stereo_signal, channels=2, rate=32000)
    stereo = subprocess.check_output([sys.executable, str(script), str(stereo_a), str(stereo_b)], text=True)
    assert "lag=17 frames" in stereo and "sample-difference RMS=0.00" in stereo
    mismatch = subprocess.run([sys.executable, str(script), str(stereo_a), str(stereo_rate)],
                              text=True, capture_output=True)
    assert mismatch.returncode != 0 and "matching channel counts and sample rates" in mismatch.stderr

print("audio capture comparison: ok")
