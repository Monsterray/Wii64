#!/usr/bin/env python3
"""Compare aligned 16-bit PCM WAV captures; differences are not a quality score."""

import array
import argparse
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
    if len(samples) != frames * channels:
        raise ValueError(f"{path}: truncated WAV")
    if sys.byteorder != "little":
        samples.byteswap()
    return channels, rate, frames, samples


def active_region(samples, channels):
    first = last = None
    for i, value in enumerate(samples):
        if value:
            first, last = (i // channels if first is None else first), i // channels
    return (first, last + 1) if first is not None else None


def compare(path_a, path_b, max_lag_ms=100, segment_frames=1024,
            start_s=None, duration_s=None, lag_frames=None):
    ch_a, rate_a, frames_a, a = read_wav(path_a)
    ch_b, rate_b, frames_b, b = read_wav(path_b)
    if (ch_a, rate_a) != (ch_b, rate_b):
        raise ValueError("WAVs must have matching channel counts and sample rates")
    active_a, active_b = active_region(a, ch_a), active_region(b, ch_b)
    if not active_a or not active_b:
        if active_a == active_b and start_s is None and duration_s is None:
            return {"lag": 0, "frames": 0, "rms": 0.0, "peak": 0, "correlation": None}
        raise ValueError("both WAVs must contain a non-silent region")

    start_a = active_a[0] if start_s is None else int(start_s * rate_a)
    length = (min(segment_frames, active_a[1] - start_a, active_b[1] - active_b[0])
              if duration_s is None else int(duration_s * rate_a))
    if length <= 0 or start_a < 0 or start_a + length > frames_a:
        raise ValueError("selected interval is outside the first WAV")
    lag_limit = rate_a * max_lag_ms // 1000
    low = max(0, start_a - lag_limit)
    high = min(frames_b - length, start_a + lag_limit)
    if lag_frames is not None:
        low = high = start_a + lag_frames
    if low > high:
        raise ValueError("selected interval is outside the second WAV or lag bound")
    if low < 0 or high + length > frames_b:
        raise ValueError("selected interval is outside the second WAV")

    offset = next((i for i in range(length)
                   if any(a[(start_a + i) * ch_a + c] for c in range(ch_a))), None)
    if offset is None:
        raise ValueError("selected interval is silent")
    align_length = min(1024, length - offset)

    # Minimize squared error across candidate offsets; ties choose the earliest.
    def error(start_b):
        return sum((a[(start_a + i) * ch_a + c] - b[(start_b + i) * ch_a + c]) ** 2
                   for i in range(offset, offset + align_length) for c in range(ch_a))

    start_b = min(range(low, high + 1), key=error)
    power = peak = a_power = b_power = cross = 0
    for i in range(length):
        for c in range(ch_a):
            left = a[(start_a + i) * ch_a + c]
            right = b[(start_b + i) * ch_a + c]
            difference = left - right
            power += difference * difference
            peak = max(peak, abs(difference))
            a_power += left * left
            b_power += right * right
            cross += left * right
    return {
        "lag": start_b - start_a,
        "frames": length,
        "rms": math.sqrt(power / (length * ch_a)),
        "peak": peak,
        "correlation": cross / math.sqrt(a_power * b_power) if a_power and b_power else None,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=pathlib.Path)
    parser.add_argument("second", type=pathlib.Path)
    parser.add_argument("--start-s", type=float, help="seconds from the first WAV's start")
    parser.add_argument("--duration-s", type=float, help="seconds to compare (default: 1024 frames)")
    parser.add_argument("--lag-frames", type=int, help="use a known offset instead of searching")
    args = parser.parse_args()
    if ((args.start_s is not None and (not math.isfinite(args.start_s) or args.start_s < 0))
            or (args.duration_s is not None and (not math.isfinite(args.duration_s) or args.duration_s <= 0))):
        parser.error("start must be nonnegative and duration must be positive")
    try:
        result = compare(args.first, args.second, start_s=args.start_s,
                         duration_s=args.duration_s, lag_frames=args.lag_frames)
    except (OSError, ValueError, wave.Error) as exc:
        sys.exit(str(exc))
    similarity = (f"{result['correlation']:.3f}" if result['correlation'] is not None else "n/a")
    print(f"aligned lag={result['lag']} frames, compared={result['frames']} frames, "
          f"sample-difference RMS={result['rms']:.2f}, peak={result['peak']}, "
          f"correlation={similarity} "
          "(not an audible-quality rating)")


if __name__ == "__main__":
    main()
