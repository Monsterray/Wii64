# Fatal DSI with ROM VM active

- Hardware job: `20260930-173721-7f52a0`, exit 0, 47 seconds.
- Artifact: `agent-crash` in `../2026-09-30_paging-artifacts.json`.
- Config: DK64, dynarec, neutral replay, `agent_crash_vi=60`.
- Expected write: `0x10`; observed DSI (3), DAR `00000010`.
- Matching ELF identifies PC `80057940` as `devAgent_testCrash`; LR resolves
  to `new_vi` at `main/timers.c:143`. LTO did not retain a source line for PC.
- HBC-Reborn 1.8.6 returned automatically; no power-off or manual recovery.

This validates fatal-DSI delegation while the ROM VM is active. It is not a
performance baseline or proof that every exception type recovers. The ordinary
profiling run leaves the deliberate crash disarmed.
