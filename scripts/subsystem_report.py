#!/usr/bin/env python3
"""Inclusive subsystem survey; hot-operation estimates are not additive CPU totals."""
import argparse
import json
from pathlib import Path

from chain_table import parse_game


def load(path):
    path = Path(path)
    if path.is_dir():
        path /= "perf.log"
    rows, stages = [], {}
    with path.open(encoding="latin-1") as log:
        for line in log:
            line = line.replace("\0", "").strip()
            if line.startswith("subsystem_time: "):
                fields = dict(part.split("=", 1) for part in line.split()[1:])
                name = fields.pop("stage")
                stages[name] = {key: int(value) for key, value in fields.items()}
            elif line.startswith("game: "):
                row = parse_game(line)
                row["subsystems"] = stages
                rows.append(row)
                stages = {}
    return rows


def milliseconds(stage):
    calls, timed = stage.get("calls", 0), stage.get("timed_calls", 0)
    return stage.get("timed_us", 0) * calls / timed / 1000 if timed else None


def speed(row):
    return row.get("vis", 0) * 1e6 / row["wall_us"] / row["vi_rate"]


def report(rows):
    for row in rows:
        wall_ms = row["wall_us"] / 1000
        print(f"\n{Path(row['rom']).name}: {row['vis']} VIs, {speed(row):.3f}x, "
              f"requested sleep {100 * row.get('sleep_us', 0) / row['wall_us']:.1f}%")
        stages = row["subsystems"]
        if not stages:
            print("  subsystem probes absent; counts/speed cannot identify an operation bottleneck")
            continue
        for name, stage in sorted(stages.items(), key=lambda item: milliseconds(item[1]) or 0, reverse=True):
            ms = milliseconds(stage)
            if ms is None:
                print(f"  {name:18} {stage['calls']:9} calls; no completed timing samples")
                continue
            exact = stage["calls"] == stage["timed_calls"]
            label = "observed" if exact else "estimate"
            few = " [few samples]" if stage["timed_calls"] < 20 else ""
            alias = " [sampling estimate exceeds wall time]" if not exact and ms > wall_ms else ""
            print(f"  {name:18} {ms:9.1f} ms {label:8} ({100 * ms / wall_ms:5.1f}% wall), "
                  f"{stage['timed_calls']}/{stage['calls']} timed{few}{alias}")
        limiter = stages.get("limiter", {})
        if limiter.get("timed_calls") and limiter["timed_calls"] == limiter["calls"]:
            actual, requested = limiter["timed_us"], row.get("sleep_us", 0)
            print(f"  limiter observed-requested: {(actual - requested) / 1000:+.1f} ms")
    print("\nInclusive spans overlap (dispatch includes lookup/compile; execution can include RSP/limiter). "
          "Do not sum them. Timing includes interruptions and waits; sampled estimates can alias.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    rows = load(args.run)
    if not rows:
        parser.error("no completed game rows")
    if args.json:
        print(json.dumps(rows, indent=2))
    else:
        report(rows)
