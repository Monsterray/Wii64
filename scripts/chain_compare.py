#!/usr/bin/env python3
"""chain_compare.py A B

Compare two chained runs game by game. A and B are each a baseline id (its rows in
baselines/games.csv) or a run directory (its perf.log, as .dev/wii64_diag.sh chain or a
Wii's SD card left it). Games are matched by ROM file name. For each game: speed, idle %,
fps, exceptions, recompiles, underruns, IPC, and the B/A ratio.

Exceptions and recompiles repeat to within a count or two run to run under Dolphin for
the same build and chain (randomize_interrupt jitter), so a larger move is the build.
"""
import csv, os, pathlib, re, sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from chain_table import games

ROOT = pathlib.Path(__file__).parent.parent
METRICS = ["speed", "idle_pct", "avg_fps", "exceptions", "recompiles", "treeDepthMax", "underruns", "ipc"]


def load(src):
    p = pathlib.Path(src)
    if p.is_dir():
        out = {}
        for g in games(p / "perf.log"):
            wall = g.get("wall_us", 0) / 1e6
            rate = g.get("vi_rate") or (50.0 if re.search(r"\((E|Europe|PAL)\)", g["rom"], re.I) else 60.0)
            g["speed"] = g.get("vis", 0) / wall / rate if wall else 0
            g["idle_pct"] = 100 * g.get("sleep_us", 0) / g["wall_us"] if g.get("wall_us") else 0
            g["ipc"] = g["pmc2"] / g["pmc1"] if g.get("pmc1") else 0
            out[os.path.basename(g["rom"])] = g
        return out
    with open(ROOT / "baselines" / "games.csv", newline="", encoding="utf-8") as f:
        rows = [r for r in csv.DictReader(f) if r["id"] == src]
    if not rows:
        sys.exit(f"{src}: not a directory and no rows with that id in baselines/games.csv")
    return {r["rom"]: {k: float(v) if re.fullmatch(r"-?[0-9.]+", v or "") else v for k, v in r.items()} for r in rows}


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    a, b = load(sys.argv[1]), load(sys.argv[2])
    print(f"A = {sys.argv[1]}\nB = {sys.argv[2]}\n")
    for rom in list(a) + [r for r in b if r not in a]:
        print(rom)
        if rom not in a or rom not in b:
            print("  only in " + ("B" if rom not in a else "A") + "\n")
            continue
        for m in METRICS:
            x, y = float(a[rom].get(m) or 0), float(b[rom].get(m) or 0)
            ratio = f"{y / x:6.3f}" if x else "     -"
            print(f"  {m:<13} {x:>12.3f} {y:>12.3f}  B/A {ratio}")
        print()


if __name__ == "__main__":
    main()
