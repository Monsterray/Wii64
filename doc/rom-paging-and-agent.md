# ROM paging and HBC development agent

## MEM2 budget

Wii64 keeps its fixed regions below `0x933E0000`. IOS owns memory above the
usable boundary; the Wii's physical 64 MiB is not an application budget.
See [WiiBrew's memory map](https://wiibrew.org/wiki/Memory_map).
Startup checks the SDK's runtime boundary and reserves the fixed regions in
Arena2. `arena2_free` now excludes those regions.

| Layout | Reserved | Unclaimed before SDK allocations |
|---|---:|---:|
| Before this change | 51.146 MiB | 763,904 bytes |
| With the NAND buffer | 51.178 MiB | 731,136 bytes |
| With the development agent | 51.182 MiB | 727,040 bytes |

The ROM cache remains 16 MiB, including a 64 KiB page-table region. The TLB
tables retain 8 MiB and the texture cache retains 15 MiB. Increasing the ROM
cache also requires changes to the VM's 12-bit physical-page index; changing
one size constant is not sufficient. Measure texture and JIT high-water marks
before taking capacity from another region.

HBC-Reborn keeps its log target at `0x91800000` and its crash record at
`0x91800020`. Agent builds reserve a 4 KiB gap there, before the texture cache.
The 32 KiB NAND buffer occupies previously unclaimed MEM2 and is IPC-aligned.
Run `bash .dev/test_rom_vm.sh` to check both layouts and the loader/cache.

## Development agent

Keep HBC-Reborn in an external checkout beside Wii64, like libogc2 and libfat.
Set `WII64_HBC_ROOT` if it is elsewhere. In a devkitPro shell:

```bash
bash .dev/build_agent.sh
bash .dev/build.sh glN64_wii clean
bash .dev/build.sh glN64_wii HBC_AGENT=1
```

For another location, also pass `HBC_AGENT_ROOT=/path/to/hbc-reborn` to the Wii64
build. Quote paths with spaces. These steps use the same compiler and libogc2
as Wii64 on macOS and in Windows' devkitPro MSYS2 shell. This Intel Mac uses
devkitPPC r50-1 and libogc2 r2464. The SDK helper probes the installed headers
for renamed exception-frame fields and the removed `MQ_ERROR_SUCCESSFUL`
constant. It keeps the SDK's frame-offset assertions active and rebuilds its
small archive when flags change. HBC-Reborn source is not copied or changed.

The agent is off in ordinary builds. Development builds provide status,
mounted-device file requests, cooperative exit, and fatal-exception records.
ROM staging completes before agent initialization. The agent starts networking
asynchronously when needed; result uploads wait for that startup to finish.
VM saves and restores the prior DSI entry and delegates faults outside its
managed range, failed NAND I/O, and failed PTE insertion to that entry.

The HOME overlay is not enabled. It needs about 1.8 MiB of transient MEM1;
spare MEM2 is not a substitute for a tested framebuffer integration. Keep
transfers out of timed gameplay because they consume CPU and heap capacity.

Use [the leased hardware runner](hardware-session.md). When the external
`tools/hbc.py` is present, the runner verifies that HBC itself answers, retains
before/after status, and checks for a new Wii64 crash while collecting results.
It writes `crash.json` and `crash.txt`, with source lines from the DOL's matching
ELF. It preserves a previous crash report instead of clearing it. Status polls
are five seconds apart; `agent-status.json` is bounded to the latest response.
It checks for a new crash again after HBC returns, including cleanup faults
after the final result upload.

For an expected-crash regression, use an agent profiling build:

```bash
bash .dev/test_agent_wii.sh /path/to/frozen-build.dol
```

Keep its ELF beside it. This queued test loads DK64 (>16 MiB, VM active), then
`agent_crash_vi=60` deliberately writes to `0x10`. It passes only if the report
identifies `devAgent_testCrash` and HBC returns. The probe exists only with both
`HBC_AGENT=1` and `PERF_PROF`. Normal runs leave it disarmed. Never power off the
Wii at the end of a job.

The first hardware regression passed: job `20260930-173721-7f52a0` captured
DSI (3), DAR `00000010`, and the `devAgent_testCrash` symbol while DK64's VM
was active. HBC 1.8.6 returned without manual recovery. See
`baselines/2026-09-30_agent-crash/`. LTO can omit a source line even when the
matching ELF identifies the function.

## Paging experiment

The reference now checks every NAND seek/read/write and rejects truncated ROM
loads. Failed initialization and repeated teardown are safe. Fatal I/O no longer
resumes with stale ROM bytes.

The candidate has two opt-in build flags:

- `VM_ROM_PREFLUSH=1`: write remaining dirty ROM pages after loading, before
  gameplay. Coalesce up to 16 physically and virtually consecutive pages.
- `VM_PAGE_READAHEAD=1`: fetch an aligned eight-page window on a NAND read miss.
  Cache already byte-swapped bytes, invalidate on writes and VM reset, and
  publish a window only after an exact read. Mapping/replacement policy stays
  unchanged.

Both flags remain off until hardware checks justify the defaults. Preflush
changes when writeback happens; it does not eliminate that startup cost.

```bash
bash .dev/profile_rom_paging.sh
```

This reuses the survey driver for frozen candidate/reference/candidate builds
and central-lease jobs. Both builds use the same agent and subsystem probes.
Set `WII64_SURVEY_QUEUE_ONLY=1` to submit without waiting. Use
`bash .dev/profile_subsystems.sh --collect /path/to/survey-directory` to collect
those jobs later without rebuilding; it verifies the frozen artifact hashes
and restores the comparison mode from the saved metadata. Use
`scripts/subsystem_compare.py --same-probes` for these triples; ordinary probe
overhead comparisons still require full/disabled/full.

The added `vm_fault`, `vm_victim`, `vm_read`, and `vm_write` timers cover all
accepted VM faults. `vm_io` records reads, cache hits, transferred bytes, writes,
and errors. They reset at the first guest VI, so gameplay measurements exclude
loading and preflush. Zero I/O errors and complete VI targets are prerequisites
for a performance result. Inclusive timers overlap; do not sum them.

For raw-SD Dolphin runs, stage the config with folder sync first, then use raw
mode without diagnostic arguments. The launcher rejects ignored arguments and
clears extracted results before boot so an old log cannot complete a new run.
Host playback stays muted while the guest sound engine runs.

## First Dolphin result

Both runs used 900 guest VIs, neutral replay, dynarec, reference audio settings,
LLE DSP, and the same isolated SD profile. All six entries completed, with zero
NAND I/O errors. These are title/intro scenes, not whole-game results.

| Scene | Reference speed | Candidate speed | Runtime ROM-copy stall |
|---|---:|---:|---|
| Mario Party 3 PAL | 0.877x | 1.000x | 2,652 ms to 169 ms |
| TWINE | 0.826x | 0.989x | 3,365 ms to 307 ms |
| DK64 | 0.844x | 0.999x | 3,040 ms to 234 ms |

TWINE's runtime NAND reads fell from 371 to 77; DK64's fell from 335 to 58.
Their remaining load-time dirty writebacks fell to zero during gameplay. The
reference and candidate are filed in `baselines/2026-09-30_dolphin_paging_*`.
Artifact hashes and source differences are recorded in
`baselines/2026-09-30_paging-artifacts.json`. The preflush build also changed
agent network startup, so repeat with the frozen survey before attributing
all wall-time changes to one flag.
Keep Dolphin and Wii numbers separate. Hardware results and crash recovery
must be filed before enabling the experimental defaults.

## Read-ahead-only Wii comparison

Reference/read-ahead/reference runs completed the same three scenes, returned
to HBC, and reported no I/O errors or audio overruns. The reference repeats
agreed within 0.1% in wall time. Relative to their mean:

| Scene | Reference speed | Read-ahead speed | Speed gain |
|---|---:|---:|---:|
| Mario Party 3 PAL | 0.873x | 0.910x | 4.3% |
| TWINE | 0.830x | 0.885x | 6.7% |
| DK64 | 0.847x | 0.894x | 5.6% |

This candidate did not preflush. Dirty writebacks invalidated each window, so
there were zero cache hits and eight times as many bytes read. That is not
the final paging configuration. Results are filed in
`baselines/2026-09-30_wii_paging_{reference_a,readahead,reference_b}`.

## Next checks

1. Complete the queued combined preflush/read-ahead run; fatal DSI passed.
2. Run the matched survey from one source tree, then test longer gameplay and
   ROM switching. Include a small fully cached ROM and a 64 MiB paged ROM.
3. Measure startup writeback separately from gameplay stalls. Retain ordinary
   release checks with probes and the agent off.
4. Measure texture/JIT occupancy before changing MEM2 sizes. After paging is
   under control, rank observed RSP/graphics/compile spans; do not choose the
   next layer from aliased hot-operation estimates alone.

The matched survey is queued in
`.dev/runs/subsystem-survey-20260930-180820-T054`. Its collector is running;
reports and the three-way comparison are written there after the jobs finish.
These jobs share the central queue with other workstations. Their results are
not yet acceptance evidence.
