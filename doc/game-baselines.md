# Game compatibility baselines

Autoboot smoke-test results for the 5 ROMs in the shared test set, using
`.dev/wii64_diag.sh` / `.dev/wii64_soak.sh` (`autoboot_rom=` + optionally
`dynarec_trace=1` in `sd:/wii64/diag.cfg`). CPU core under test: **Dynarec**
(the default), Dolphin `CPUThread=True` (dual-core), `DSPHLE=False` (LLE).

Re-run this sweep after any change that touches ROM loading, `cpu_init`,
the dynarec dispatch loop, or CIC/PIF boot handling, and update this table --
it's the regression net for exactly that class of bug.

| ROM | CIC | Result | Notes |
|---|---|---|---|
| Super Mario 64 | 6102 | **PASS** | Boots straight to the title screen, 60 VI/s. Reliable across every run this session. |
| Pokemon Snap | 6101/6102 | **PASS** | Boots straight into the intro cutscene at 60/60. Reliable across every run this session. |
| Banjo-Kazooie (U) | **6105** | **INTERMITTENT** | Observed both a full hang (needing force-kill) and two clean boots to the title screen, from the *same build*, on consecutive runs. Not a deterministic per-ROM failure. |
| Zelda: Majora's Mask (E) | **6105** | **INTERMITTENT** | Not re-tested as thoroughly as OoT MQ this session, but shares the same CIC and the same dynarec, so treat as the same bug class until shown otherwise. |
| Zelda: Ocarina of Time Master Quest | **6105** | **INTERMITTENT / STALLS OFTEN** | Hung on most runs this session, sometimes recovered by the new watchdog (see below), sometimes not (see "Known gap"). |

## The bug: an intermittent, non-deterministic dynarec hang on CIC-6105 games

All three known-affected ROMs are CIC-NUS-6105 games; the two reliable ones
are not. But **this is not a deterministic "PC X always hangs" bug** --
early in this investigation `dynarec_trace` runs looked like they hung at
identical dispatch counts and PCs across the Zelda games, but that turned
out to be an artifact of the trace tool's own sampling cap (it stopped
*drawing* well before the game actually stopped *running* -- see
`main/dynarec_trace.c`'s comments for the embarrassing details). Once the
tool was fixed to keep sampling much further, the same ROM was observed
taking completely different execution paths -- and different apparent
"stuck" states -- across separate runs of the identical build. Confirmed
concretely:

- A wild jump to **PC = 0x00000000**, which is never a legitimate address in
  normal operation, repeated indefinitely.
- A **period-2 oscillation** between PC=0 and an otherwise ordinary-looking
  RDRAM address (0x800023A0 in one capture), which a naive "same address as
  last dispatch" check can't catch (the address is technically "new" on
  every single iteration).
- At least one further stall (Zelda: OoT Master Quest, one run) that didn't
  resolve within ~200s of real time even with the watchdog below active,
  meaning there is likely a **third failure shape** not yet characterized --
  possibly a longer-period cycle than the watchdog's 16-slot detection
  window, or a hang outside the dynarec dispatch loop entirely (RSP-HLE
  audio processing, an SI/PIF wait, etc.).

Root cause (why a wild jump lands on exactly 0, or what makes a
normal-looking block cycle forever) is **not found**. What CIC-6105 games
have in common structurally is the PIF challenge/response exchange
(`gc_memory/n64_cic_nus_6105.c`, `gc_memory/pif.c`'s `cic_challenge`
handling) and its SI-interrupt-driven completion -- tested and ruled OUT one
specific theory (the `randomize_interrupt` jitter in
`add_random_interupt_time`, up to 63 cycles added to every PI/SI interrupt)
by disabling it entirely via diag.cfg's `randomize_interrupt=0` and seeing
the exact same hang, so the non-determinism comes from somewhere else --
most likely genuine host-timing jitter (Dolphin's own dual-core thread
scheduling, SD-image read latency) interacting with something timing-
sensitive in the CIC-6105 boot path, rather than anything Wii64 controls
directly.

## The fix: a dynarec watchdog (partial)

`r4300/ppc/Wrappers.c`'s `dynarec()` now detects both confirmed failure
shapes and recovers gracefully -- stops emulation and returns to the menu
with a message, instead of hanging Dolphin (confirmed to also block a
graceful Dolphin close, since the wedged guest never cooperates with the
power-down request):

- `DYNAREC_WATCHDOG_ZEROPC_LIMIT` (64 dispatches): PC == 0 specifically,
  low threshold since it's unambiguous.
- `DYNAREC_WATCHDOG_CYCLE_LIMIT` (300,000): a 16-slot ring buffer detects
  the current dispatch address reappearing within recent history (catches
  the period-2 case and anything else up to period 16), generous threshold
  so it can't misfire on a real bounded loop (e.g. a large memory clear).

**Known gap**: confirmed via direct screenshot that the PC==0 case recovers
correctly. The cycle-detector has NOT been confirmed to trigger within
reasonable time on the third stall shape above (a Zelda: OoT MQ run stayed
wedged -- CPU busy, screen unchanged -- for ~200s past boot without the
watchdog firing). Likely candidates if picking this back up: raise
`DYNAREC_WATCHDOG_RING_SIZE` past 16, or add a wall-clock-based fallback
(e.g. "no NEW address seen in N real seconds") that doesn't depend on
guessing the cycle's period at all.

## Reproducing / testing

Prefer `.dev/wii64_soak.sh` over hand-rolled sleep+screenshot loops -- it's
one call that starts the ROM, polls status+screenshot on its own schedule,
and ends with a single STALLED/IDLE/RUNNING/CRASHED verdict instead of a
screenshot per check:

```bash
.dev/wii64_soak.sh wii64-Rice.dol 200 "autoboot_rom=sd:/wii64/roms/<rom>"
# add "dynarec_trace=1" as an extra arg to also get an on-screen PC trace
```

For manual control (`start`/`shot`/`status`/`stop`/`killstop`), see
`.dev/wii64_diag.sh`. A "STALLED" verdict with the screen still on
"Starting ROM: [...]" means the watchdog hasn't fired (yet, or at all) --
check the screenshot before concluding it's the third, uncaught shape.
