"""Find how fast a capture plays a song relative to a reference recording.

audio_tempo.py REF.wav CAPTURE.wav [start_s duration_s]
Onset envelopes (spectral flux, 100 frames/s) are cross-correlated while the
capture is stretched by factors 0.6-1.6; the best factor is capture speed /
reference speed (1.25 = the capture plays the song 25% fast), and where in the
capture the reference starts. Needs ffmpeg.
"""
import shutil, subprocess, sys
import numpy as np

FF = shutil.which("ffmpeg") or r"C:\Program Files (x86)\ffmpeg\bin\ffmpeg.exe"
SR, HOP = 8000, 80  # 100 envelope frames per second


def load(path, start=None, dur=None):
    cmd = [FF, "-loglevel", "error"]
    if start is not None:
        cmd += ["-ss", str(start), "-t", str(dur)]
    cmd += ["-i", path, "-ac", "1", "-ar", str(SR), "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, capture_output=True, check=True).stdout, np.float32)


def envelope(x):
    n = 512
    frames = np.lib.stride_tricks.sliding_window_view(x, n)[::HOP] * np.hanning(n)
    mag = np.log1p(np.abs(np.fft.rfft(frames, axis=1)))
    flux = np.maximum(np.diff(mag, axis=0), 0).sum(axis=1)
    flux -= np.convolve(flux, np.ones(50) / 50, "same")  # remove slow loudness changes
    return np.maximum(flux, 0)


def best_corr(a, b):
    """Max normalized cross-correlation of b sliding over a."""
    a = (a - a.mean()) / (a.std() + 1e-9)
    b = (b - b.mean()) / (b.std() + 1e-9)
    n = 1 << int(np.ceil(np.log2(len(a) + len(b))))
    c = np.fft.irfft(np.fft.rfft(a, n) * np.conj(np.fft.rfft(b, n)), n)
    return c.max() / len(b), int(c.argmax())


def tempo(ref, cap):
    """[(correlation, speed, lag in stretched frames)], best first."""
    results = []
    for f in np.arange(0.6, 1.6, 0.0025):
        # Stretch the capture by f: a capture that plays fast (f > 1) gets longer.
        stretched = np.interp(np.arange(0, len(cap) - 1, 1 / f), np.arange(len(cap)), cap)
        if len(stretched) > len(ref):
            r, lag = best_corr(stretched, ref)
        else:
            r, lag = best_corr(ref, stretched)
        results.append((r, f, lag))
    return sorted(results, reverse=True)


def main():
    ref = envelope(load(sys.argv[1]))
    cap = envelope(load(sys.argv[2], *(sys.argv[3:5] if len(sys.argv) > 4 else (None, None))))
    for r, f, lag in tempo(ref, cap)[:5]:
        print(f"speed {f:.4f}  corr {r:.3f}  starts at {lag / 100 / f:.2f} s")


if __name__ == "__main__":
    main()
