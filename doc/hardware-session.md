# Hardware session: one boot, every measurement

On a real Wii the slow part of a measurement is moving the SD card between the workstation
and the console. So one boot collects everything: every ROM's baseline, then the stick
check. The Wii powers itself off when it is done, and everything is on the card.

## What is on the card

| Path | What |
|---|---|
| `apps/wii64/boot.dol` | a **PERF_PROF** build (`.dev/build_profiling.sh glN64_wii` or `Rice_wii`) -- a release build compiles every probe out and writes no `perf.log` |
| `wii64/diag.cfg` | the chain: `scripts/chains/hardware.txt`, copied as is |
| `wii64/roms/` | the ROMs `hardware.txt` names, under exactly those file names |

Nothing else is needed. A chain never loads or writes the card's saves (autosave is off for
the whole boot), so a card with real saves on it is safe and the runs are comparable.

## On the Wii

Boot Wii64 from the Homebrew Channel and leave it. No controller is needed: each game runs
for a fixed number of its own VIs, the stick check drives port 1 itself, and the Wii powers
off at the end. With `hardware.txt` as shipped, that takes about 7 minutes.

If a game hangs (Majora's Mask does, on this build: CIC-6105), a watchdog on the host
retrace cuts it off at 3x its length plus 60 s and marks it `how=timeout`; the chain goes
on to the next game. If the Wii is still on long after, it hung somewhere the watchdog
cannot reach -- pull the card anyway: every finished game's results are already on it.

## Back on the workstation

Copy the card's `wii64/` folder into a run directory, then:

```bash
python scripts/chain_table.py <run dir>                     # one row per game; xfb_NN.png screenshots
python scripts/padtest.py <run dir>/padtrace_05.csv         # the stick check (game 5 in hardware.txt)
python scripts/baseline_add.py --chain <run dir> --platform hardware --plugin glN64 \
    --id 2026-MM-DD_hw_glN64 --purpose "hardware.txt, first hardware baseline"
python scripts/chain_compare.py 2026-09-22_dolphin_all_glN64 2026-MM-DD_hw_glN64
```

## What the hardware numbers add

Dolphin answers what the code does; only the Wii answers how long it takes. The columns
that mean something only on hardware:

- **speed / idle** -- `idle` is the share of wall time the frame limiter slept: the
  headroom. `speed` under 1.00 with `idle` at 0% means that game does not run full speed
  on a Wii. Under Dolphin these describe the workstation.
- **ipc** (`pmc2 / pmc1`) -- Broadway's performance counters: instructions completed per
  cycle. Dolphin does not count instructions completed, so this is 0 there. Other events
  can be selected at build time (`-DPMC_MMCR0=...`, see `main/perf_prof.c`).
- **underruns** -- audible audio gaps. Dolphin's DSP timing is not the Wii's.
- **flush_us** -- what writing `perf.log` itself cost; it should stay a few ms per game.
  If it is large, the SD card is slow and the probes are disturbing the run.

## Running the same chain in Dolphin

```bash
.dev/build_profiling.sh glN64_wii && cp wii64-glN64.dol .dev/wii64-glN64-prof.dol
.dev/wii64_diag.sh chain .dev/wii64-glN64-prof.dol scripts/chains/hardware.txt .dev/runs/hw_dolphin
```

Chains run in their own Dolphin profile (`.dev/dolphin_runs`), so they do not collide with
an interactive session. Build rules: clean between plugins (AGENTS.md section 3).
