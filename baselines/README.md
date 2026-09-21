# Baselines

Reference `perf.log` captures kept so a later change can be compared against a known point
instead of judged from memory. One directory per run, filed by `scripts/baseline_add.py`.
Layout ported from WiiStation's `baselines/` (project on the same machine, same idea).

```
baselines/index.csv          one row per baseline with its key metrics (open it in a spreadsheet)
baselines/<id>/perf.log      the captured PERF_PROF perf.log
baselines/<id>/notes.md      what this run was, how it was captured, the numbers in one page
```

## Capturing a run

Needs a PERF_PROF build (release builds compile every probe to a no-op):

```bash
.dev/build_profiling.sh Rice_wii                       # clean + rebuild with -DPERF_PROF
.dev/wii64_diag.sh start wii64-Rice.dol "stress_selectrom=1"   # or any diag.cfg line(s) that exercise the path you're measuring
sleep 15                                                # let it run long enough to matter
.dev/wii64_diag.sh perf out.log                         # force-kills, reads perf.log straight off the raw SD image
```

`perf` reads the raw SD image (`dolphin_profile/Load/WiiSD.raw`) with `scripts/sdimage_read.py`
instead of waiting on Dolphin's graceful close -- this project's builds don't reliably reach a
clean unmount (see `doc/subsystem-review.md`'s shutdown-hang writeup), so the usual
synced-folder copy of `perf.log` is stale or missing; the raw image isn't.

## Filing a run

```
python scripts/baseline_add.py out.log --id 2026-09-21_selectrom_baseline --purpose "boxart page load, 5 ROMs, stock"
```

Picks up the perf.log's page/tile averages itself (same parsing as `perf_compare.py`); writes
`notes.md` and appends a row to `index.csv`. `--force` replaces an existing id.

## Comparing against one

```
python scripts/perf_compare.py baselines/<id>/perf.log <new capture>.log
```

Prints both logs' averaged tile-load/page-load numbers side by side plus the B/A ratio -- a
regression or improvement is visible at a glance instead of eyeballed from two raw logs.

## What the numbers are, and are not

Captured under Dolphin unless the notes say otherwise: its guest clock is an instruction-count
estimate, good for ranking relative cost and catching a regression, not a promise of what real
hardware would show. A hardware capture (same PERF_PROF build, real Wii, `sd:/wii64/perf.log`
pulled off a real SD card) files the same way and should say `hardware` in its purpose.

Right now the only real-time-bearing data `perf_prof.c` emits is the ROM browser's boxart page
load (`pageBegin`/`tileLoaded`/`pageEnd` -- see `menu/SelectRomFrame.cpp`'s
`selectRomFrame_FillPage`). Extending it to cover CPU-core/dynarec or renderer timing the way
WiiStation's `Gamecube/perf_prof.c` does for its own domain is future work, not done yet.
