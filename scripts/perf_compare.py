#!/usr/bin/env python3
"""perf_compare.py A.log B.log
Side-by-side comparison of two sd:/wii64/perf.log files (PERF_PROF builds
only -- see .dev/build_profiling.sh and .dev/wii64_diag.sh's `perf`
subcommand). Averages:
  - every "page" boxart-load event (a page is `page: tiles=N loadLimit=N`
    through the matching `page end: ...` -- see main/perf_prof.h and
    menu/SelectRomFrame.cpp's selectRomFrame_FillPage), and
  - every `vis:`/`fps:` gameplay speed sample (main/timers.c's
    new_vi()/new_frame(), ~2/sec during actual gameplay -- the same
    numbers the in-game "Show FPS" overlay shows: vis is the raw
    VI-interrupt rate, fps is the completed-display-list rate), and
  - the last `cpu: exceptions=N cacheResets=N` line (cumulative counters
    since boot, bumped from r4300/exception.c/r4300.c -- see
    main/perf_prof.c's perfProf_cpuSample doc comment)
and prints A, B and B/A so a regression or improvement is visible at a
glance. A metric missing from both logs is skipped. Pattern ported from
WiiStation's scripts/perf_compare.py (project on the same machine),
reworked for Wii64's page/tile/mark/vis/fps log format instead of
WiiStation's tagged counter blocks.
"""
import re, sys

PAGE_RE = re.compile(r"^page: tiles=(\d+) loadLimit=(\d+)")
TILE_RE = re.compile(r"^\s*tile\s+(\d+): real=(\d+) init=(\d+)us load=(\d+)us flush=(\d+)us")
END_RE = re.compile(r"^page end: invalidate=(\d+)us total=(\d+)us")
VIS_RE = re.compile(r"^vis: ([\d.]+)")
FPS_RE = re.compile(r"^fps: ([\d.]+)")
CPU_RE = re.compile(r"^cpu: exceptions=(\d+) cacheResets=(\d+)")

def pages(path):
    out, cur = [], None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = PAGE_RE.match(line)
        if m:
            cur = {"tiles": int(m.group(1)), "loadLimit": int(m.group(2)), "tile": []}
            continue
        m = TILE_RE.match(line)
        if m and cur is not None:
            cur["tile"].append({"real": int(m.group(2)), "init": int(m.group(3)),
                                 "load": int(m.group(4)), "flush": int(m.group(5))})
            continue
        m = END_RE.match(line)
        if m and cur is not None:
            cur["invalidate"] = int(m.group(1)); cur["total"] = int(m.group(2))
            out.append(cur); cur = None
    return out

def speed_samples(path):
    vis, fps = [], []
    for line in open(path, encoding="utf-8", errors="replace"):
        m = VIS_RE.match(line)
        if m: vis.append(float(m.group(1))); continue
        m = FPS_RE.match(line)
        if m: fps.append(float(m.group(1)))
    return vis, fps

def cpu_totals(path):
    exceptions = cache_resets = 0
    for line in open(path, encoding="utf-8", errors="replace"):
        m = CPU_RE.match(line)
        if m:
            exceptions, cache_resets = int(m.group(1)), int(m.group(2))
    return exceptions, cache_resets

def avg(vals):
    return sum(vals) / len(vals) if vals else 0.0

def summarize(path):
    pgs = pages(path)
    real_tiles = [t for p in pgs for t in p["tile"] if t["real"]]
    vis, fps = speed_samples(path)
    exceptions, cache_resets = cpu_totals(path)
    return {
        "pages": len(pgs),
        "avg_tile_init_us": avg([t["init"] for t in real_tiles]),
        "avg_tile_load_us": avg([t["load"] for t in real_tiles]),
        "avg_tile_flush_us": avg([t["flush"] for t in real_tiles]),
        "avg_page_total_us": avg([p["total"] for p in pgs]),
        "avg_page_invalidate_us": avg([p["invalidate"] for p in pgs]),
        "vis_samples": len(vis),
        "avg_vis": avg(vis),
        "min_vis": min(vis) if vis else 0.0,
        "fps_samples": len(fps),
        "avg_fps": avg(fps),
        "min_fps": min(fps) if fps else 0.0,
        "total_exceptions": exceptions,
        "total_cache_resets": cache_resets,
    }

KEYS = ["pages", "avg_tile_init_us", "avg_tile_load_us", "avg_tile_flush_us",
        "avg_page_total_us", "avg_page_invalidate_us",
        "vis_samples", "avg_vis", "min_vis", "fps_samples", "avg_fps", "min_fps",
        "total_exceptions", "total_cache_resets"]

def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    a, b = sys.argv[1], sys.argv[2]
    A, B = summarize(a), summarize(b)
    print(f"{'metric':<24}{'A':>12}{'B':>12}{'B/A':>8}")
    printed = False
    for key in KEYS:
        va, vb = A[key], B[key]
        if not va and not vb:
            continue
        printed = True
        ratio = f"{vb / va:.3f}" if va else "-"
        print(f"{key:<24}{va:>12.1f}{vb:>12.1f}{ratio:>8}")
    if not printed:
        sys.exit("no comparable data (no page-load events or vis/fps samples) in either log")

if __name__ == "__main__":
    main()
