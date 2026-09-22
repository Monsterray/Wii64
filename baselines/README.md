# Baselines

Reference `perf.log` captures kept so a later change can be compared against a known point
instead of judged from memory. One directory per run, filed by `scripts/baseline_add.py`.
Layout ported from WiiStation's `baselines/` (project on the same machine, same idea).

```
baselines/index.csv          one row per baseline with its key metrics (open it in a spreadsheet)
baselines/<id>/perf.log      the captured PERF_PROF perf.log
baselines/<id>/notes.md      what this run was, how it was captured, the numbers in one page
```

## Chained runs (the main kind now)

A chain (`diag.cfg` `chain=` lines, `scripts/chains/`) runs several games in one boot, each
for a fixed number of its own VIs, and leaves one `game:` line per game in `perf.log`, a
screenshot per game (`xfb_NN.bin`) and a pad trace when there was pad input. Same chain,
same build: exceptions and recompiles repeat to within a count or two under Dolphin, so a
bigger move is the build. See `doc/hardware-session.md` for running one on a Wii.

```bash
.dev/build_profiling.sh glN64_wii && cp wii64-glN64.dol .dev/wii64-glN64-prof.dol
.dev/wii64_diag.sh chain .dev/wii64-glN64-prof.dol scripts/chains/all.txt .dev/runs/all_glN64
python scripts/baseline_add.py --chain .dev/runs/all_glN64 --plugin glN64     --id 2026-09-22_dolphin_all_glN64 --purpose "all.txt, stock settings"
python scripts/chain_compare.py 2026-09-22_dolphin_all_glN64 .dev/runs/<new run>
```

```
baselines/games.csv          one row per game per chained baseline (platform, plugin, build, speed, idle %, ...)
baselines/<id>/              perf.log, diag.cfg (the chain), run.log, padtrace_NN.csv, notes.md
baselines/media/<id>/        xfb_NN.png screenshots -- kept out of git (.gitignore), as in WiiStation
```

`games.csv` columns worth knowing: `speed` = guest VIs per wall second / the ROM's VI rate
(1.00 = full speed); `idle_pct` = share of wall time the frame limiter slept, the headroom;
`ipc` = instructions per cycle from Broadway's counters (hardware only, 0 in Dolphin);
`underruns` = audible audio gaps; `treeDepthMax` = the dynarec's worst function-tree
lookup depth; `flush_us` = what writing `perf.log` itself cost.

## Capturing a single run

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
python scripts/baseline_add.py out.log --id 2026-09-21_banjo_gameplay_baseline --rom "Banjo-Kazooie" --purpose "20s autoboot gameplay, dynarec"
```

Picks up whichever metrics the perf.log actually has (same parsing as `perf_compare.py` --
boxart page/tile averages, gameplay vis/fps averages, or both); writes `notes.md` and appends a
row to `index.csv`. `--force` replaces an existing id.

## Comparing against one

```
python scripts/perf_compare.py baselines/<id>/perf.log <new capture>.log
```

Prints both logs' averages side by side plus the B/A ratio -- a regression or improvement is
visible at a glance instead of eyeballed from two raw logs. Only metrics present in at least one
of the two logs are printed, so a boxart-page capture compared against a gameplay capture just
shows nothing in common (expected -- they don't measure the same thing).

## What the numbers are, and are not

Captured under Dolphin unless the notes say otherwise: its guest clock is an instruction-count
estimate, good for ranking relative cost and catching a regression, not a promise of what real
hardware would show. A hardware capture (same PERF_PROF build, real Wii, `sd:/wii64/perf.log`
pulled off a real SD card) files the same way and should say `hardware` in its purpose.

`min_vis`/`min_fps` will usually be a near-zero outlier from the very first sample of a capture
(the window from ROM boot to the first 500ms tick, not a real stutter) -- expect it, don't read
it as a hitch unless it recurs later in the log too.

Three real-time/count-bearing sources exist:
- The ROM browser's boxart page load (`pageBegin`/`tileLoaded`/`pageEnd` --
  `menu/SelectRomFrame.cpp`'s `selectRomFrame_FillPage`).
- Actual gameplay speed (`visSample`/`fpsSample`, ~2/sec -- `main/timers.c`'s
  `new_vi()`/`new_frame()`, the same numbers the in-game "Show FPS" overlay shows: `vis` is the
  raw VI-interrupt rate, `fps` is the completed-display-list rate). This is the number that
  answers "did this change make the emulator faster or slower" for real gameplay, the way
  WiiStation's vblacks-per-guest-second `speed` metric does for its own domain.
- CPU-core counters (`cpuSample`, same ~2/sec cadence as `visSample` -- `total_exceptions` from
  `r4300/exception.c`'s `exception_general()`, `total_cache_resets` from `r4300/r4300.c`'s `go()`
  reinitializing the recompiler cache). Cumulative since boot, not a rate: a baseline compares
  a run's *total* count, the same way WiiStation's `jit_full`/`jit_part`/`exc` counters are used.
  A `total_cache_resets` above 1 for a normal single-ROM run is itself a signal (the JIT cache
  had to fully reinitialize mid-session) worth looking at directly in the raw log, not just the
  averaged number.

Renderer-internal timing (Rice_GX/glN64_GX present time, texture cache hit/miss -- what
WiiStation's `Gamecube/perf_prof.c` also tracks for its GPU side) isn't captured yet. It would
need counters added inside the renderer plugins' own draw/present paths rather than a call site
that already existed to hook into, the same way the CPU counters above needed new increments
inside `exception_general()`/`go()` instead of just wrapping an existing call. Future work.
