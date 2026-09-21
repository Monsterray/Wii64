#!/usr/bin/env python3
"""perf_compare.py A.log B.log
Side-by-side comparison of two sd:/wii64/perf.log files (PERF_PROF builds
only -- see .dev/build_profiling.sh and .dev/wii64_diag.sh's `perf`
subcommand). Averages every "page" boxart-load event in each log (a page
is `page: tiles=N loadLimit=N` through the matching `page end: ...` --
see main/perf_prof.h and menu/SelectRomFrame.cpp's selectRomFrame_FillPage)
and prints A, B and B/A so a regression or improvement is visible at a
glance. Pattern ported from WiiStation's scripts/perf_compare.py (project
on the same machine), reworked for Wii64's page/tile/mark log format
instead of WiiStation's tagged counter blocks.
"""
import re, sys

PAGE_RE = re.compile(r"^page: tiles=(\d+) loadLimit=(\d+)")
TILE_RE = re.compile(r"^\s*tile\s+(\d+): real=(\d+) init=(\d+)us load=(\d+)us flush=(\d+)us")
END_RE = re.compile(r"^page end: invalidate=(\d+)us total=(\d+)us")

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

def avg(vals):
    return sum(vals) / len(vals) if vals else 0.0

def summarize(pgs):
    real_tiles = [t for p in pgs for t in p["tile"] if t["real"]]
    return {
        "pages": len(pgs),
        "avg_tile_init_us": avg([t["init"] for t in real_tiles]),
        "avg_tile_load_us": avg([t["load"] for t in real_tiles]),
        "avg_tile_flush_us": avg([t["flush"] for t in real_tiles]),
        "avg_page_total_us": avg([p["total"] for p in pgs]),
        "avg_page_invalidate_us": avg([p["invalidate"] for p in pgs]),
    }

def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    a, b = sys.argv[1], sys.argv[2]
    pa, pb = pages(a), pages(b)
    if not pa or not pb:
        sys.exit("no complete page-load events found in one or both logs "
                  "(a page needs its matching 'page end' line -- a killed "
                  "instance mid-load won't have one)")
    A, B = summarize(pa), summarize(pb)
    print(f"{'metric':<24}{'A':>12}{'B':>12}{'B/A':>8}")
    for key in ["pages", "avg_tile_init_us", "avg_tile_load_us", "avg_tile_flush_us",
                "avg_page_total_us", "avg_page_invalidate_us"]:
        va, vb = A[key], B[key]
        ratio = f"{vb / va:.3f}" if va else "-"
        print(f"{key:<24}{va:>12.1f}{vb:>12.1f}{ratio:>8}")

if __name__ == "__main__":
    main()
