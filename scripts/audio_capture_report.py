#!/usr/bin/env python3
"""Report signal level in Dolphin's opt-in WAV captures; no audio is modified."""

import array
import math
import pathlib
import sys
import wave


def measure(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2:
            raise ValueError(f"{path}: expected 16-bit PCM")
        samples = nonzero = power = peak = 0
        while data := wav.readframes(8192):
            chunk = array.array("h")
            chunk.frombytes(data)
            if sys.byteorder != "little":
                chunk.byteswap()
            samples += len(chunk)
            nonzero += sum(value != 0 for value in chunk)
            power += sum(value * value for value in chunk)
            peak = max(peak, max((abs(value) for value in chunk), default=0))
        return wav.getnchannels(), wav.getframerate(), wav.getnframes(), nonzero, peak, math.sqrt(power / samples) if samples else 0


if __name__ == "__main__":
    paths = [wav for root in map(pathlib.Path, sys.argv[1:]) for wav in sorted(root.rglob("*.wav"))]
    if not paths:
        sys.exit("usage: audio_capture_report.py <capture-dir> [...]; no WAV files found")
    for path in paths:
        channels, rate, frames, nonzero, peak, rms = measure(path)
        print(f"{path}: {frames / rate:.2f}s, {rate} Hz, {channels} ch, nonzero={nonzero}, peak={peak}, RMS={rms:.1f}")
