#!/usr/bin/env python3
"""Fail a hardware smoke run if a game stalls or ends on a black frame."""

import pathlib
import sys

from chain_table import games


def check(run):
    rows = games(run / "perf.log")
    if len(rows) != 2:
        return f"expected 2 games, got {len(rows)}"
    for row in rows:
        n = int(str(row["n"]).split("/")[0])
        if row.get("how") != "vis":
            return f"game {n}: ended with {row.get('how')}"
        frame = run / f"xfb_{n:02d}.bin"
        if not frame.is_file():
            return f"game {n}: no final frame"
        data = frame.read_bytes()
        if len(data) < 32 or data[:4] != b"WXFB":
            return f"game {n}: invalid final frame"
        luma = data[16::64]  # one Y sample per 32 pixels of YUYV
        dark = sum(y < 25 for y in luma) / len(luma)
        if row.get("avg_fps", 0) < 2:
            return f"game {n}: render stalled ({row.get('avg_fps', 0):.1f} DL/s)"
        if dark > 0.95:
            return f"game {n}: final frame is {dark:.0%} black"
    return None


if __name__ == "__main__":
    problem = check(pathlib.Path(sys.argv[1]))
    print("SMOKE FAIL: " + problem if problem else "SMOKE PASS: both games rendered")
    sys.exit(bool(problem))
