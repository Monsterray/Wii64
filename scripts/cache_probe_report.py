#!/usr/bin/env python3
"""Summarize opt-in cache probes; never convert sampled timings into FPS gains."""
import argparse
import json
from pathlib import Path

from chain_table import parse_game


def fields(text):
    return {key: int(value) for key, value in (item.split("=", 1) for item in text.split())}


def read(path):
    rows, pending = [], {}
    for line in Path(path).read_text(encoding="latin-1").replace("\0", "").splitlines():
        for tag in ("cache_texture", "cache_xfb"):
            if line.startswith(tag + ": "):
                if tag in pending:
                    raise ValueError(f"{path}: duplicate {tag} before game result")
                pending[tag] = fields(line.split(": ", 1)[1])
        if line.startswith("mark: cache_gx_test: "):
            pending["gx_test"] = fields(line.split(": ", 2)[2])
        if line.startswith("game: "):
            if not {"cache_texture", "cache_xfb"} <= pending.keys():
                raise ValueError(f"{path}: game result has missing cache probes")
            game = parse_game(line)
            texture, xfb = pending["cache_texture"], pending["cache_xfb"]
            if not {"mip_calls", "levels", "staged_bytes", "packed_bytes", "timed_calls", "sampled_us"} <= texture.keys():
                raise ValueError(f"{path}: incomplete texture counters")
            if not {"calls", "checks", "stale_checks"} <= xfb.keys():
                raise ValueError(f"{path}: incomplete XFB counters")
            if any(value < 0 for record in pending.values() for value in record.values()):
                raise ValueError(f"{path}: negative probe counter")
            if texture["packed_bytes"] > texture["staged_bytes"] or xfb["stale_checks"] > xfb["checks"]:
                raise ValueError(f"{path}: inconsistent probe counters")
            if "gx_test" in pending and (pending["gx_test"].get("fresh_errors", 1) or pending["gx_test"].get("guard_errors", 1)):
                raise ValueError(f"{path}: private GX ownership test failed")
            rows.append(dict(log=str(path), game=game, **pending))
            pending = {}
    if pending or not rows:
        raise ValueError(f"{path}: no complete cache probe results or unfinished game")
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", nargs="+", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    try:
        rows = [row for path in args.logs for row in read(path)]
    except (ValueError, OSError) as error:
        parser.error(str(error))
    if args.json:
        print(json.dumps(rows, indent=2))
        return
    print("ROM | mip calls/levels | staged/packed KiB | sampled mip us | XFB stale/checks | framebuffer KiB")
    for row in rows:
        texture, xfb = row["cache_texture"], row["cache_xfb"]
        framebuffer = (f"{xfb['framebuffer_bytes']/1024:.1f}"
                       if "framebuffer_bytes" in xfb else "not recorded")
        print(f"{Path(row['game']['rom']).name} | {texture['mip_calls']}/{texture['levels']} | "
              f"{texture['staged_bytes']/1024:.1f}/{texture['packed_bytes']/1024:.1f} | "
              f"{texture['sampled_us']} ({texture['timed_calls']} samples) | "
              f"{xfb['stale_checks']}/{xfb['checks']} | "
              f"{framebuffer}")
        if "gx_test" in row:
            print("  Private GX ownership test:", row["gx_test"])
    print("Sampled times are diagnostic residence, not wall-time shares or performance gains.")


if __name__ == "__main__":
    main()
