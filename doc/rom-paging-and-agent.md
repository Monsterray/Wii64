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

## ROM paging defaults

The reference now checks every NAND seek/read/write and rejects truncated ROM
loads. Failed initialization and repeated teardown are safe. Fatal I/O no longer
resumes with stale ROM bytes.

Wii builds enable both paging improvements by default:

- `VM_ROM_PREFLUSH=1`: write remaining dirty ROM pages after loading, before
  gameplay. Coalesce up to 16 physically and virtually consecutive pages.
- `VM_PAGE_READAHEAD=1`: fetch an aligned eight-page window on a NAND read miss.
  Cache already byte-swapped bytes, invalidate on writes and VM reset, and
  publish a window only after an exact read. Mapping/replacement policy stays
  unchanged.

Use `-DVM_ROM_PREFLUSH=0 -DVM_PAGE_READAHEAD=0` in `DEBUG_FLAGS` to build the
old paging path for a comparison. Clean before changing flags. Fully cached
ROMs skip preflush; GameCube's ARAM loader is unchanged. Preflush
writes all remaining dirty mapped pages, including pages that a short scene
might never evict. It can add startup NAND traffic, not just move writeback.

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
Profiling builds also emit `ROMCache_load: elapsed_us=...` outside the gameplay
clock. Preflush emits its own duration and, with subsystem probes, write count
and byte count. This adds two clock reads per measured load/flush, not per VI.

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
are recorded below. Longer runs test switching and later scenes; short intro
timings do not establish compatibility.

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

## First combined Wii run

Job `20260930-173721-0d8b3e` completed all three 900-VI scenes and returned to
HBC. There were zero I/O errors, zero overruns, and zero gameplay NAND writes.
Relative to the mean of the earlier references:

| Scene | Reference speed | Combined speed | Speed gain | ROM-copy stall |
|---|---:|---:|---:|---:|
| Mario Party 3 PAL | 0.873x | 0.990x | 13.4% | 2,589 ms to 185 ms |
| TWINE | 0.830x | 0.988x | 19.1% | 3,274 ms to 314 ms |
| DK64 | 0.847x | 0.988x | 16.7% | 2,954 ms to 252 ms |

NAND reads fell to 41/77/58, with 251/294/277 cache hits. Underruns were
1/1/3, versus reference counts of 1/17/11. The whole job took 175 seconds,
versus about 134 seconds for the references. That includes loading, uploads,
and HBC return, so it is not an isolated preflush timing. The frozen build
predates the explicit load/preflush duration and byte-count marks.

The data is filed in `baselines/2026-09-30_wii_paging_preflush`. The source
differences in that first artifact manifest still apply. The matched survey
below isolates the combined paging change.

## Matched Wii comparison

Candidate/reference/candidate used one source tree, the same HBC SDK, and
the same probes. All nine entries completed 900 VIs, with no paging errors
or audio overruns. Every job returned to HBC 1.8.6.

| Scene | Reference speed | Candidate mean | Wall-time reduction | Underruns, reference to candidate |
|---|---:|---:|---:|---:|
| Mario Party 3 PAL | 0.874x | 0.990x | 11.8% | 1 to 1 |
| TWINE | 0.829x | 0.988x | 16.1% | 16 to 1 |
| DK64 | 0.842x | 0.988x | 14.8% | 12 to 3 |

The candidate repeats agreed within 0.1% in wall time. CPU cycles changed by
less than 0.5%; the gain comes mainly from removing synchronous NAND waits,
not from faster guest instructions. These are neutral intro/title scenes.
Logs are filed in `baselines/2026-09-30_wii_paging_matched_*`; artifact hashes
and queue IDs remain in `baselines/2026-09-30_paging-artifacts.json`.

## Longer checks and startup cost

The `rom_paging_long` chain passed on Dolphin and the Wii with 2,400 VIs of
SM64, 3,600 each of MP3 PAL/TWINE/DK64, then 900 VIs of SM64 again. SM64 uses
its start replay; MP3 presses A periodically. All targets completed, with no
NAND I/O errors or audio overruns. The Wii returned to HBC 1.8.6. The first
hardware attempt lacked the two replay files and correctly failed validation;
the runner now stages them through the leased HBC file-transfer path.

| ROM | Wii load | Included preflush | Longer-scene speed |
|---|---:|---:|---:|
| SM64 | 1.970 s | Not needed | 0.995x |
| MP3 PAL | 39.261 s | 17.245 s | 0.997x |
| TWINE | 29.925 s | 17.386 s | 0.995x |
| DK64 | 29.897 s | 17.409 s | 0.996x |

Each large ROM preflush wrote 16,711,680 bytes. Timed gameplay wrote none.
These are longer scripted scenes and switching checks, not a full-game
compatibility test. Preserve startup costs separately when comparing versions.
Logs are filed in `baselines/2026-09-30_{wii,dolphin}_paging_long/`.

## Next checks

1. Extend `rom_paging_long` with recorded gameplay routes and a 64 MiB ROM.
   No 64 MiB ROM is available in this workstation's test collection yet.
2. Measure startup writeback separately from gameplay stalls. Retain ordinary
   release checks with probes and the agent off.
3. Measure texture/JIT occupancy before changing MEM2 sizes. After paging is
   under control, rank observed RSP/graphics/compile spans; do not choose the
   next layer from aliased hot-operation estimates alone.

The matched survey completed in
`.dev/runs/subsystem-survey-20260930-180820-T054`. `scripts/subsystem_report.py`
now separates ROM-load time and preflush duration/bytes from gameplay timings
when the log contains those marks. Preflush is included in total ROM-load
time; adding the two durations would count it twice.
