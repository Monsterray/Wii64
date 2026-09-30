#!/usr/bin/env python3
"""Compare aligned 16-bit PCM WAV captures; differences are not a quality score."""

import array
import math
import pathlib
import sys
import wave


def read_wav(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getcomptype() != "NONE" or wav.getsampwidth() != 2:
            raise ValueError(f"{path}: expected uncompressed 16-bit PCM")
        channels, rate, frames = wav.getnchannels(), wav.getframerate(), wav.getnframes()
        samples = array.array("h", wav.readframes(frames))
    if sys.byteorder != "little":
        samples.byteswap()
    return channels, rate, frames, samples


def active_region(samples, channels):
    first = last = None
    for i, value in enumerate(samples):
        if value:
            first, last = (i // channels if first is None else first), i // channels
    return (first, last + 1) if first is not None else None


def compare(path_a, path_b, max_lag_ms=100, segment_frames=1024):
    ch_a, rate_a, frames_a, a = read_wav(path_a)
    ch_b, rate_b, frames_b, b = read_wav(path_b)
    if (ch_a, rate_a) != (ch_b, rate_b):
        raise ValueError("WAVs must have matching channel counts and sample rates")
    active_a, active_b = active_region(a, ch_a), active_region(b, ch_b)
    if not active_a or not active_b:
        if active_a == active_b:
            return {"lag": 0, "frames": 0, "rms": 0.0, "peak": 0}
        raise ValueError("both WAVs must contain a non-silent region")

    start_a = active_a[0]
    length = min(segment_frames, active_a[1] - start_a, active_b[1] - active_b[0])
    if length <= 0:
        raise ValueError("no active samples to compare")
    lag_limit = rate_a * max_lag_ms // 1000
    low = max(active_b[0], start_a - lag_limit)
    high = min(active_b[1] - length, start_a + lag_limit)
    if low > high:
        raise ValueError("active regions do not overlap within the lag bound")

    # Minimize squared error across candidate offsets; ties choose the earliest.
    def error(start_b):
        return sum((a[(start_a + i) * ch_a + c] - b[(start_b + i) * ch_a + c]) ** 2
                   for i in range(length) for c in range(ch_a))

    start_b = min(range(low, high + 1), key=error)
    differences = [a[(start_a + i) * ch_a + c] - b[(start_b + i) * ch_a + c]
                   for i in range(length) for c in range(ch_a)]
    return {
        "lag": start_b - start_a,
        "frames": length,
        "rms": math.sqrt(sum(value * value for value in differences) / len(differences)),
        "peak": max(map(abs, differences), default=0),
    }


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: audio_capture_compare.py A.wav B.wav")
    try:
        result = compare(*map(pathlib.Path, sys.argv[1:]))
    except (OSError, ValueError, wave.Error) as exc:
        sys.exit(str(exc))
    print(f"aligned lag={result['lag']} frames, compared={result['frames']} frames, "
          f"sample-difference RMS={result['rms']:.2f}, peak={result['peak']} "
          "(not an audible-quality rating)")


if __name__ == "__main__":
    main()
