# Zelda checks, 2026-10-01

The previous boot failure did not recur in the current glN64 build. The user
also reports that the old issue is gone. No new engine fix is claimed here.

Build 1.6.6 completed these checks with dynarec, accurate synthesis, DSP output,
accurate mixing, stable latency and native audio sync. Dolphin used MMU and DSP
LLE. Its host playback was muted; the guest sound engine remained enabled.

| Check | Master Quest | Majora's Mask (Europe) |
|---|---|---|
| 900-VI Wii boot | Title screen; 96.13% speed | Completed; 97.50% speed |
| 3,600-VI Wii menu replay | Name entry | Name entry |
| 7,200-VI Wii new-file replay | Flat grey final capture; needs follow-up | Forest opening cutscene |
| 3,600-VI Dolphin menu replay | Completed with graphics and audio tasks | Completed with graphics and audio tasks |

Speed is completed VIs divided by wall time and the nominal VI rate. A
completed test is not a full-game compatibility result. The menu checks did
not use existing saves. The new-file replay operates in RAM; diagnostic chains
do not write game saves.

The old September glN64 binary stalled in both Majora's Mask and Super Mario 64
before producing a game row. It is not a valid Zelda-specific comparison.
A retained 1.6.2 build also booted Majora's Mask, with slower ROM paging.
Total guest exceptions include interrupts; that count does not prove a CIC fault.

The HBC version changed externally from 1.8.6 to 1.8.9 between runs. The linked
agent SDK remained 1.8.6. Do not attribute cross-session timing changes to an
emulator patch without a matched control.

## Repeat

Build with `PERF_PROF` and `PERF_SUBSYSTEM_PROBES`, then keep the DOL beside its
matching ELF. Use the dedicated cold-boot, menu or new-file checks:

```bash
bash .dev/zelda_check.sh .dev/runs/zelda-1.6.7/wii64-glN64.dol new_game
```

The full-library sweep extends both Zelda entries to 9,000 VIs. See
[library testing](library-testing.md) for its scope and measurement limits.
