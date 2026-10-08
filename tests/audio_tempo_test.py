"""Run with python3 tests/audio_tempo_test.py: a click pattern played 20% fast reads 1.20."""
from pathlib import Path
import sys
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from audio_tempo import SR, envelope, tempo

rng = np.random.default_rng(64)
beats = np.cumsum(rng.uniform(0.15, 0.5, 60))  # an irregular rhythm, so only one alignment fits


def clicks(times, seconds):
    x = np.zeros(int(seconds * SR), np.float32)
    for t in times:
        i = int(t * SR)
        x[i:i + 200] += np.sin(np.arange(200) * 0.9) * np.exp(-np.arange(200) / 40)
    return x


ref = envelope(clicks(beats, beats[-1] + 1))
fast = envelope(clicks(2.0 + beats / 1.2, 2.0 + beats[-1] / 1.2 + 1))  # starts 2 s in
best = tempo(ref, fast)[0]
assert abs(best[1] - 1.2) < 0.01 and abs(best[2] / 100 / best[1] - 2.0) < 0.05, best
print(f"audio tempo: 1.20x clicks read {best[1]:.4f}x at {best[2] / 100 / best[1]:.2f} s: ok")
