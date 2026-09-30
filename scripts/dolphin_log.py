#!/usr/bin/env python3
"""Bound timed-test logs and print the latest boot tail with constant memory."""
import argparse
from collections import deque
import gzip
from pathlib import Path
import sys


def latest_tail(path, lines=20):
    tail = deque(maxlen=lines)
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", errors="replace") as log:
        for line in log:
            if "Starting core = Wii mode" in line:
                tail.clear()
            tail.append(line)
    return "".join(tail)


def within_budget(path, mib):
    return not path.exists() or path.stat().st_size <= mib * 1024 * 1024


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path)
    parser.add_argument("--max-mib", type=int, help="check size instead of printing the tail")
    args = parser.parse_args()
    if args.max_mib is not None:
        if args.max_mib < 1:
            parser.error("--max-mib must be positive")
        if not within_budget(args.path, args.max_mib):
            sys.exit(f"Dolphin log exceeds {args.max_mib} MiB: {args.path}")
    else:
        print(latest_tail(args.path), end="")
