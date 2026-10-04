# Optimization audit: 2026-10-03

Started from clean fork `master` at `6378be6`, verified against origin. Read
AGENTS, the build/test skill, subsystem survey, CPU plan, recent GPU results,
and prior subsystem review before choosing work. This is a targeted pass, not
a claim that every subsystem or every instruction is fully audited.

Started candidate searches with the local call graph, then checked targeted
source and history around invalidation, eviction and cache lifetimes. The graph
refresh passes its MCP smoke test but has partial parse coverage; unresolved
files require direct source checks, not inferred missing call paths.

## Working candidates

| Rank | Candidate | Type | Evidence | Expected gain | Risk | Scope | Test |
|---|---|---|---|---|---|---|---|
| P0, fixed | Cross-instruction write invalidation | BUG | Actual walker leaves `[0x80000004,0x80000008)` compiled after writing three bytes at `0x80000002` | Correct instruction coverage; no speed claim | Extra invalidation where previously missed; hot-helper cost needs Wii measurement | Range helper and both PI DMA modes | Byte oracle, sanitizers, both walker flags, ROM checks |
| P0, fixed | BSD queued snapshot filename | BUG | Existing offline queue test fails with `mktemp: ... File exists` | Restores repeatable Mac queue entry | Low; snapshot is already read explicitly by Bash | One filename template | Existing offline queue test |
| P0, fixed | SDK output paths with spaces | BUG | Actual SDK make splits absolute OUT/BUILD paths; minimal real-make fixture also fails | Restores the documented SDK builder on this Mac | Low; temporary output, same final archive path | SDK launcher and offline fixture | Actual SDK build and spaced-path fixture |
| P2 | Post-JR dispatch/execute ranking | PERF | Earlier dispatch ranking predates `f254d63`; current four-scene survey queued | Unknown until refreshed measurements | Probe overhead and sparse-sample aliasing | Existing probes, no new engine code | Full/control/full, scene gates and alternate interval if needed |
| P2 | Compile-time dispatch-table scan | PERF | `Wrappers.c` scans 1,024 entries after compiling a function | Unknown; cold compilation may dominate this cost | Removing the scan can leave stale code pointers | One compile/invalidation boundary | Measure compile frequency/self cost before a lifetime-preserving experiment |
| P2 | Allocation-pressure lifetime audit | GUARD | `Recompile.c`'s overlap walker holds node pointers across malloc retries that call `release()` | Crash prevention if eviction can remove a held node | Not reproduced; a broad allocator rewrite is not justified | One overlap branch under injected allocation failure | Extract the production branch; force eviction and check its held-node lifetime |

No PERF candidate was enabled from source intuition or old timings. Stop this
pass's production changes here: remaining leads need hardware data or a tighter
failure seam. Do not replay the resolved tree/heap, JR, mixer or paging work.

## Invalidation mechanism

`invalidate_func_range(addr, bytes)` receives byte counts from generated
grouped stores (`MIPS-to-PPC.c`, `check_memory_range`) and slow stores, plus PI
DMA. The old test compared against the original `addr, addr+4, ...` walker.
Both implementations missed a trailing instruction when the write started
unaligned. A byte-by-byte oracle exposes that shared defect.

The fix rounds the range to touched instruction words, not whole pages:
`bytes/4 + (bytes%4 + (addr&3) + 3)/4`, then aligns the start down. Division
before addition prevents count overflow. Zero bytes return without invalidation.
The maximum count is 1,073,741,825, below `UINT_MAX`. The long walker stays
within one page per query; address wrap occurs only when advancing to the next
page. Functions have word-aligned start/end, so this does not free a function
untouched by any written byte. Preserve the one/two-word short path and the
already-invalid-page skip.

PI DMA's disabled-fast-path branch previously duplicated the flawed stride;
both branches now use the shared helper. This is one correctness mechanism,
not a new DMA optimization. Generated PPC and interpreter semantics are not
otherwise changed. The real guest sequence still needs a hardware check.

`python3 tests/invalidation_range_test.py` compiles the production walkers and
overlap search, models freeing, and compares written-byte coverage. It checks
unaligned tails, zero sizes, page boundaries, aliases, wrapped ranges, many
functions per page and random tree orders under ASan/UBSan. The production
word-count expression is also checked against a 64-bit oracle through UINT_MAX
without traversing a four-gigabyte range.

## Fast paths checked and rejected changes

- Dispatch/JR lifetime: `invalid_code_set` marks the table stale on a page
  becoming invalid; `free_func` also marks it stale. Generated JR tests that
  flag and returns to C before using cached targets. Compilation clears every
  entry for an in-place rebuilt function. Model claims that these safeguards
  are missing were rejected against source. No speculative thread locks added.
- Texture CRC hint: the hint still uses the full legacy field predicate and
  falls back to the list. Both removal callers reach `TextureCache_FreeSlot`,
  which clears the matching hint; ROM reset clears the table. No blanket
  disable or another cache added.
- The tree rewrite, Floyd heap construction, byte-snapshot TMEM memo, limiter
  units, and batching across VTX are already investigated in durable docs.
  The memo has mixed results; batching across VTX was rejected. No repeat port.
- The GPU is not the established bottleneck. Do not remove GX waits or port
  Kart's billboard copy to EFB on the assumption that fewer CPU calls preserve
  the displayed-frame semantics. Current CPU copies remain unchanged.
- RSP envelope/mixer mode hoisting is already implemented and covered by exact
  PCM/state hashes. The existing audio regression suite passes; no new DSP
  arithmetic or paired-single synthesis introduced.

Selective references were pinned, not ported:

- [Not64 `d35a7a1`, `r4300/ppc/Wrappers.c`](https://github.com/extremscorner/not64/blob/d35a7a1c454eca973995939dbca3f06725cf4a6e/r4300/ppc/Wrappers.c)
  uses point invalidation for grouped stores. It is not an independent oracle
  for complete written-byte coverage.
- [Mupen64Plus core `ba95bab`, `cached_interp.c`](https://github.com/mupen64plus/mupen64plus-core/blob/ba95bab92a76744753bfe61470823a4937850ab0/src/device/r4300/cached_interp.c)
  invalidates page blocks with a different representation and zero-size
  contract. Copying that implementation would change Wii64's cache semantics.

No reference code was borrowed. The fix follows the local byte-coverage invariant.

## Builds and measurements

SDK source: HBC-Reborn `0b214c7a78d81d87f4a2b24e53feb3c967e26d5c` (1.9.4).
The shared checkout remains unchanged at 1.8.8; another project owns the Wii.
A temporary external checkout at `/private/tmp/wii64-hbc-sdk.0pDjdc` supplied
headers and the client; Wii64's archive lives in `.dev/hbc_agent/`. Retain that
source until queued jobs finish. Builds use GCC 16.1.0 and external libogc2/
libfat. Pass `HBC_AGENT_ROOT` to make while the shared checkout is older.
The new launcher fixes output paths, not an automatic shared-source upgrade.

Host subsystem, ROM/VM and RSP/audio suites pass. Both glN64 and Rice Wii targets
clean-build. Muted MMU/LLE release smoke checks for both pass without invalid-access,
DSP or SD-sync warnings. Existing signedness warnings remain; no blanket warning
suppression was added. GameCube runtime and Wii U hardware were not tested.

The complete local Dolphin collection uses frozen 1.6.11 DOL/ELF files with the
CPU fix, reference audio and existing replays. Later 1.6.12 changes only the SDK
builder and version label. Host builds overlap the library run, so its speed
numbers must not be treated as measured performance regressions or Wii gains.
This library binary lacks subsystem counters; absent values are unknown, not
zero. Use the separately queued survey to rank subsystem costs.
Library results: `.dev/runs/library-glN64_wii-20261003-210903-Nu05`.
All 18 ROMs complete their requested VI target, without launcher failures or
paging errors. All have nonzero DSP captures and nonflat final frames. Viewed
Mario Kart's race, Super Mario 64's castle view and Majora's Mask's cutscene;
Master Quest's final capture shows intro text. These checks do not certify every
pixel or audible output.

The strict library gate exits 1: all 18 record less than 100% VI/wall speed
(99.17–99.98%), 15 record audio underruns and four record slow VI windows.
There are 142 underruns, zero overruns. Nine ROMs shared with the recent
`library-20261002b-dolphin` baseline have identical recompile counts; exception
counts vary by -4 to +115. Master Quest records 78 underruns versus that
baseline's 48. This cross-host comparison is not a matched A/B test; retain the
warning without attributing it to this fix. No confirmed regression or
performance improvement is established; hardware validation is pending.

The new full/control/full Wii survey froze runtime source `bf2a6e8`, the same
SDK, matching DOL/ELF files, flags and chain. It measures current CPU behavior
and probe cost, not candidate/reference optimization gain:

- Directory: `.dev/runs/subsystem-survey-20261003-210947-L9Sp`.
- Full 1: `20261003-211109-a491b1`.
- Control: `20261003-211109-6cf69e`.
- Full 2: `20261003-211109-dc0a5d`.
- Collection: `bash .dev/profile_subsystems.sh --collect "$PWD/.dev/runs/subsystem-survey-20261003-210947-L9Sp"`.

These jobs are pending behind another project's live lease. No Wii request was
sent without a lease, no other's job was cancelled, and no hardware speedup or
recovery result is claimed. Do not change these frozen files or their SDK path.

Local reviews used Muse Glimmer, Gemma4, Qwen Coder and Devstral with 10,000-token
caps and 300-second timeouts, in independent pairs (about 60, 44 and 31 seconds).
Several claims contradicted visible guards or basic arithmetic. Tests and
source settled them; model agreement is not a correctness proof. Improve local
review by attaching exact source/commit and executable arithmetic checks.

## Next experiment

Update 2026-10-04: [the queued survey and crash recovery now pass on hardware](hardware-validation-2026-10-04.md).
Use that refreshed ranking and its probe/alias limits, not the older dispatch
ranking. This does not add a speedup claim for the invalidation fix or replace
a targeted guest-instruction regression test.

The four-scene full/control/full survey is now collected. The user's follow-up
selects a [WiiStation-informed PC sampler experiment](performance-methods-2026-10-04.md)
to resolve the ranking. It is not yet implemented. No cache rewrite from the
aliased estimates.
