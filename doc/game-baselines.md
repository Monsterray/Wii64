# Game compatibility baselines

Autoboot smoke-test results for the 5 ROMs in the shared test set, captured
with `.dev/wii64_diag.sh` (`autoboot_rom=` + `dynarec_trace=1` in
`sd:/wii64/diag.cfg`) against the `wii64-Rice.dol` build at commit 3acad5b +
the fixes in this pass (Settings layout, dynarec_trace safety fix, FPS
default on). CPU core under test: **Dynarec** (the default), Dolphin
`CPUThread=True` (dual-core), `DSPHLE=False` (LLE).

Re-run this sweep after any change that touches ROM loading, `cpu_init`,
the dynarec dispatch loop, or CIC/PIF boot handling, and update this table --
it's the regression net for exactly that class of bug.

| ROM | CIC | Result | Notes |
|---|---|---|---|
| Super Mario 64 | 6102 | **PASS** | Boots straight to the title screen (Bowser intro cutscene), 60 VI/s. Soak-tested 45s: cutscene visibly progresses (Bowser pose changes), stayed at 60 VI/s throughout. |
| Pokemon Snap | 6101/6102 | **PASS** | Boots straight into the intro cutscene at 60 VI/s / 60 FPS. Soak-tested 45s: cutscene visibly progresses to a later scene, stayed at 60/60 throughout. |
| Banjo-Kazooie (U) | **6105** | **HANG** | Dynarec hangs during boot, before the first frame renders. `dynarec_trace` shows it cycling through dispatches (counter in the thousands) without ever reaching a stable state -- see below. |
| Zelda: Majora's Mask (E) | **6105** | **HANG** | Dynarec hangs at dispatch #7, PC=0xA400894C. Confirmed stuck (not just slow): identical screenshot after an extra 25s. |
| Zelda: Ocarina of Time Master Quest | **6105** | **HANG** | Dynarec hangs at the *exact same* dispatch #7, PC=0xA400894C, as Majora's Mask. |

## The pattern

All three hanging ROMs are CIC-NUS-6105 games. Both working ROMs are not.
Majora's Mask and Ocarina of Time (Master Quest) hang at the byte-identical
PC and dispatch count, which given they're both Zelda-engine titles sharing
very similar early boot code, strongly suggests **the dynarec has a bug
specific to CIC-6105 boot/PIF handling**, not a per-game issue. Banjo-Kazooie
hangs earlier (near the boot vector) rather than at PC=0xA400894C, so it may
be the same underlying bug tripped at a different point, or a related one --
not confirmed which.

This narrows "why does the dynarec hang on some games" from "could be
anything" to a specific, checkable hypothesis: something in how the dynarec
(not the pure interpreter -- see the session note that found this) handles
the CIC-6105 challenge/response path (`gc_memory/n64_cic_nus_6105.c`,
`gc_memory/pif.c`'s `cic_challenge` handling) or the memory-layout quirk
real CIC-6105 hardware needs. Next step for whoever picks this up: compare
what MIPS code actually executes around PC=0xA400894C under the pure
interpreter (which doesn't hang) against what the dynarec recompiles for the
same address.

## Reproducing

```bash
.dev/wii64_diag.sh start wii64-Rice.dol "autoboot_rom=sd:/wii64/roms/<rom>" "dynarec_trace=1"
# wait ~25-30s (bigger ROMs need longer), then:
.dev/wii64_diag.sh shot out.png
.dev/wii64_diag.sh status   # Responding + CPU% -- a hang still shows Responding=True
.dev/wii64_diag.sh killstop
```
