# Baseline findings, 2026-09-22 (Dolphin, both plugins)

Source: `baselines/games.csv`, ids `2026-09-22_dolphin_all_glN64` and
`2026-09-22_dolphin_all_Rice` (build 3eb4731, chain `scripts/chains/all.txt`: 4 ROMs x
3600 guest VIs, no input, stock settings). Dolphin only -- no hardware data yet.

## The data

| ROM | Plugin | fps | exc/s | recompiles | batches/frame | verts/frame | tree depth | underruns | MEM1 free |
|---|---|---|---|---|---|---|---|---|---|
| Super Mario 64 | glN64 | 28.2 | 455 | 1567 | 263 | 4413 | 15 | 8 | 412 KB |
| Super Mario 64 | Rice | 28.2 | 455 | 1567 | 226 | 2403 | 15 | 7 | 732 KB |
| Banjo-Kazooie | glN64 | 21.6 | 373 | 2351 | 210 | 4664 | 32 | 2 | 376 KB |
| Banjo-Kazooie | Rice | 21.6 | 373 | 2351 | 154 | 2168 | 32 | 3 | 700 KB |
| OoT Master Quest | both | 0.2 | 1959 | 480 | 0 | 0 | 9 | ~850 | -- |
| Majora's Mask (E) | both | 0.2 | 2007 | 565 | 0 | 0 | 10 | 1 | -- |

(exc/s = guest exceptions per wall second. MEM1 free = `arena1_free`, what is left in
MEM1 for the heap to grow into. Frames = avg_fps x wall time.)

## What it says

1. **Half the test set does not boot.** OoT MQ and Majora's Mask (both CIC-6105) hang
   with a black screen on both plugins, with identical core counts: the hang is in the
   core, not the renderer. New detail from these runs:
   - Both throw ~2000 exceptions/s, 4-5x a running game, and recompile only ~500
     blocks: the CPU circles a small piece of code, handling exceptions.
   - OoT MQ gets further than MM. Its audio starts and then starves (~850 gaps); MM's
     audio never starts (1 gap).
   - OoT MQ runs 50 Hz VI timing against a 60 Hz header (3600 VIs in 71.4 s). Either a
     PAL-timed ROM or a symptom of the hang -- unverified.
2. **The core is deterministic.** Same chain, same build: exceptions within 3,
   recompiles identical, across two runs and across both plugins. A later change that
   moves these numbers by more than that is real.
3. **The playable games are measured in attract mode only.** No input reaches them, so
   SM64 sits on its title screen and Banjo plays its intro. That is not gameplay load.
4. **Speed and idle are workstation numbers.** Both playable games run full speed with
   41-58% of the time asleep in the limiter -- under Dolphin. They say nothing about the
   Wii yet. IPC (PMC2/PMC1) is 0 because Dolphin does not count instructions completed.
   The PMC cycle counter itself works: 43.7 G cycles in 60 s = 729 MHz, no wrap.
5. **glN64 sends about twice the geometry of Rice for the same frames**: 4.4-4.7 k vs
   2.2-2.4 k vertices per frame, 20-40% more batches. Whether that costs anything depends
   on the Wii (CPU transform and FIFO writes vs GP time) -- unknown until hardware.
6. **MEM1 is nearly full on glN64.** About 0.4 MB left (Rice about 0.7 MB). Anything new
   that lives in MEM1 (bigger caches, buffers) has to come out of something else. MEM2
   has ~54 MB unused by the heap.
7. **Small things are small.** Rice's texture-overwrite stalls: 0-10 per minute. Audio
   gaps in the playable games: 2-8 per minute. Writing perf.log: ~5 ms per game. None of
   these is worth work on Dolphin evidence alone.
8. **FuncTree depth reaches 32 in Banjo** (15 in SM64): the degenerate BST is real, but
   its cost is unmeasured. Wait for hardware IPC before restructuring it.

## Where to focus, in order

1. **CIC-6105 boot hang.** Biggest single gain: two of four ROMs. Next step, cheap: a
   per-cause exception histogram (the Cause field in `exception_general()`) on the
   `game:` line, so the chain says which exception floods. Compare OoT MQ (gets to
   audio) against MM (does not) to bracket where boot stops. Check whether OoT MQ's
   50 Hz timing is the ROM or the hang.
2. **Hardware session** (`doc/hardware-session.md`, `scripts/chains/hardware.txt`).
   Required before any speed work: idle %, IPC and real audio gaps exist only there.
   Fix WiiStation's PMC probe first if its session shares the trip.
3. **Make the baseline represent gameplay.** Replay recorded play by VI (`chain=...,input=`
   from a Dolphin movie, `doc/controller-testing.md`; works now, no recordings yet) to get
   past title screens, and more ROMs covering
   other microcodes (F3DEX2, S2DEX) -- today the set is 2 working games in attract mode.
4. **After hardware, only if it shows a bottleneck:** glN64's geometry volume (finding
   5) if the Wii is CPU/FIFO bound in 3D scenes; the FuncTree BST (finding 8) if IPC is
   low in dispatch-heavy games.

Constraint for all of it: MEM1 headroom (finding 6).

## Not worth doing now

Vertex-format state caching, the Rice texture stall, the frame-limiter debt, audio
tuning -- all measured small, or measurable only on hardware.
