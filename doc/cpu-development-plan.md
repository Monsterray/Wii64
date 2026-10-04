# CPU development plan

Status: planning and local graph setup, 2026-10-03. This work adds developer
tooling and the 1.6.9 version label, not another CPU optimization. It is based on
fork commit `f254d63`, after rebasing onto 20 newer fork commits. Their CPU and
agent changes are retained; no speedup is attributed to this tooling commit.

## Goal and scope

Make the CPU core faster without changing N64 behavior. Cover the interpreter,
MIPS-to-PPC translator, generated code, register cache, dispatch, block lookup,
linking, code-cache eviction, invalidation, memory, TLB, CP0, FPU, interrupts,
exceptions, and CPU handoffs to DMA/RSP/plugins. RSP synthesis and graphics
optimizations are separate workstreams; CPU measurements must exclude their
children when a self-time estimate is used.

Use one phase record at a time: question, source paths, hypothesis, probe
boundaries, artifact hashes, commands, queue IDs, results, decision, next step.
Mark each phase **pending**, **active**, **passed**, or **blocked** here. Keep
failed experiments and reasons; do not enable a candidate because it builds.

## Evidence before new work

The schema-2 full/control/full captures are in
`baselines/2026-10-01_hw_subsystem_schema2_{full1,control,full2}`. Dispatch self
time was approximately 19–31% of non-sleep wall time in SM64 and DK64, and
12–18% in MK64 and Snap. Lookup is a child of dispatch, not another allocation.
Those results made dispatch/lookup a useful first CPU candidate.
Graphics still dominates some games; a CPU-only improvement cannot remove that.

The newer fork already implements `DYNAREC_DISPATCH_CACHE`, compiled
`DYNAREC_JR_LOOKUP`, and the improved invalidation walker. The final nine-scene
Wii triple records approximately 5–9% fewer CPU cycles for JR lookup plus the
invalidation changes; returns to the C dispatcher fell 82–95%. See
[the newer CPU/GPU results](gpu-results-2026-10-02.md#kept-compiled-jrjalr-read-the-dispatch-table)
and `baselines/2026-10-02_hw_jr_lookup_final_*`. These are existing fork results,
not measurements made in this Mac session. The old dispatch ranking must be
retested before choosing another optimization. A source graph will not infer
the new generated-PPC jump into a cached target.

Full probes added approximately 9–21% CPU cycles in these scenes. Execute self
time varied markedly between repeats in SM64 and DK64. It includes inline
guest memory operations, generated PPC overhead, and unclassified work. Do not
sum independent sampled estimates, infer free CPU from limiter sleep, or apply
one universal probe correction. See [probe definitions](subsystem-profile-2026-09-30.md)
and [whole-system results](subsystem-results-2026-10-01.md).

## Phases and gates

| Phase | Work | Evidence required to advance | Status |
|---|---|---|---|
| 0. Safe diagnostics | Pin the newest HBC SDK/client, validate memory layout, crash recovery, retained logs, and hang handling. Freeze an environment manifest. | Layout tests, matched ELF symbolication, expected exception/fatal/hang checks, clean HBC return. | Active: isolated SDK 1.9.4 build/layout and VM-active DSI recovery pass; shared client, fatal/hang and retained-output checks remain |
| 1. CPU map and correctness harness | Use the local graph to map active Wii branches and manual generated-code edges. Audit interpreter/comparison facilities and add bounded instruction tests. | Source-backed map and passing state comparisons for each tested instruction family. | Active: graph built; harness not implemented |
| 2. Reliable CPU baseline | Calibrate existing probes, add only the counters needed for the next decision, and repeat representative scenes. | Valid scene/replay, stable counters, measured probe cost, explicit uncertainty and CPU priority table. | Active: current four-scene survey collected; execute/helper aliasing and probe cost limit ranking |
| 3. Dispatch, lookup and code cache | Audit the existing target cache/JR lookup; measure misses, allocation, linking, eviction and recompilation. | Host stress tests, correct unlinking/invalidation, muted Dolphin checks, repeat Wii A/B/A gain. | Existing speed paths integrated; systematic correctness audit pending |
| 4. Generated PPC and register cache | Examine hot blocks, spills, helper calls, branch sequences, 64-bit arithmetic and FPU conversions. | Instruction-state comparisons and smaller/faster hot sequences without ABI, flags or FP regressions. | Pending |
| 5. Memory and translation | Separate direct RAM, TLB, slow MMIO, DMA invalidation, ROM copies and host VM faults. Measure MEM1/MEM2/cache pressure. | Alignment/alias/bounds/SMC tests, intact VM recovery, no paging errors, repeat scene gain. | Pending |
| 6. Scheduling and exceptions | Check CP0 Count/Compare, interrupt queues, delay slots, exception state, idle loops and interpreter fallback. | Correct PC/EPC/Cause/Status and event ordering in focused tests; no new timing/audio regressions. | Pending |
| 7. Library and release checks | Run all owned ROMs with fixed routes, longer sessions and ROM switching; both renderers where relevant. | No new compatibility failures, credible per-scene speed/stutter/audio results, probe-off release checks. | Active: 18-ROM glN64 Wii sweep and two-ROM Rice smoke complete; strict speed/audio warnings, Master Quest visual check and broader gameplay coverage remain |

Correctness checks apply in every phase. Phase 6 is a deeper audit, not permission
to postpone exception correctness. Reorder phases 3–6 only when phase 2 gives
stronger evidence. Fix a confirmed bug before pursuing a speed candidate.

### 0. HBC agent integration

Remote HBC-Reborn was checked at `0b214c7a78d81d87f4a2b24e53feb3c967e26d5c`
(1.9.4). The external local checkout remains 1.8.8; previous Wii64 captures used
an older SDK. These are not claims about the version currently running on the
Wii. Recheck remote HEAD and record the chosen commit before building.

The newer SDK has protocol 4, version-2 crash records, `hbc_agent_fatal()` /
`hbc_agent_fatal_now()`, `hbc_agent_alive()`, nested `hbc_agent_hold()` calls,
and `hbc.py lastlog`. Its retained log starts at `0x91800100`; the 4,140-byte
block ends at `0x9180112C`. The old 4 KiB gap overlapped the texture cache by
300 bytes. Fork commit `7ef6b8d` already fixes this with an 8 KiB gap and an
assertion for the retained log. Keep this fix; it is no longer pending work.

Current Wii builds always include the agent and HOME overlay. The fork already
calls progress heartbeats from guest-VI/menu thread context, holds expected
loads/uploads, and wraps `c_default_exceptionhandler`. The Mac's external
checkout is still 1.8.8 and lacks the required newer SDK APIs. Refresh it safely
when no live workflow uses its source, then rebuild with `.dev/build_agent.sh`.
The archive now belongs in ignored `.dev/hbc_agent/`, built with Wii64's own
compiler; the shared checkout's archive is not an acceptable substitute.

Before declaring phase 0 passed, verify the existing integration:

1. Check the aligned MEM2 reservation for **all** SDK retained blocks, with SDK-sized
   compile-time assertions and an agent/non-agent layout test. Recheck the
   Arena2 reservation and remaining budget. Do not enlarge unrelated caches.
2. Build the SDK externally with the same compiler/libogc2 as Wii64. Record
   SDK/header/archive hashes, compiler, IOS, HBC version, client and server.
   Keep old frozen artifacts unchanged. Updating the SDK is not authorization
   to install a channel/WAD or replace the shared lease server.
3. Audit progress heartbeats on the actual emulation thread. A retrace ISR
   alone must not keep a stuck CPU alive. Use balanced holds only for expected
   long loads/transfers, with cleanup on failures. Do not hold around gameplay.
   Linked code can bypass the C dispatcher; document heartbeat coverage there.
4. Add missing bounded ROM/guest-PC/phase breadcrumbs and recover the last log after
   HBC returns. Log export and console status polling stay outside timing.
5. Test exception, fatal and hang paths separately under a lease, including a
   ROM with VM active. Preserve the prior DSI handler and distinguish an
   expected ROM page fault from a fatal host fault. Never power off the Wii.

The watchdog cannot recover an interrupt-disabled deadlock. Record that limit.
Do not make synchronous networking or per-instruction logging a CPU probe.
The HOME overlay is already integrated using lent framebuffers. Preserve its
menu-thread boundary; do not revert it or reintroduce allocating frame copies.

### 1. Source map and correctness

Map `r4300/r4300.c`, `pure_interp.c`, `compare_core.c`, `ppc/Wrappers.c`,
`ppc/MIPS-to-PPC.c`, `ppc/Recompile.c`, `ppc/Register-Cache.c`, `ppc/FuncTree.c`,
`Recomp-Cache-Heap.c`, `Invalid_Code.c`, `exception.c`, `interupt.c`, and
`gc_memory/{memory,tlb,dma}.c` plus `vm/`. Record target flags from the makefiles:
`PPC_DYNAREC`, `FASTMEM`, `FASTMEM_HOT_NOFLUSH`, and `INTERPRET_*` choices.

First audit the existing `COMPARE_CORE` path rather than build another framework.
The interpreter is a comparator, not a proven oracle. Use known instruction
vectors and a trusted reference when both cores could share an error. Cover
GPR/HI/LO, signed and unsigned 32/64-bit operations, overflow, shifts/division,
branches and likely branches, delay slots, alignment, LL/SC, FPU rounding and
NaNs, CP0/TLB and exception entry/return. Save seeds and the first divergence.
For block comparisons, restore memory/device state and avoid executing MMIO
side effects twice. Match guest instruction boundaries, not merely host time
or the same number of VIs.

### 2. Probe design and cost

Reuse `dispatch`, `lookup`, `compile`, `execute_inclusive`, `cpu_helper`,
`interpreter`, `memory_slow`, `tlb_translate`, `invalidate`, `interrupt`, DMA and
VM stages. Preserve their documented nesting. Classify each new field as an
event count, sampled time, occupancy mark, or guest/host clock measurement.

Add counters only after checking existing ones: lookup hit/miss and depth
distribution, dispatch reason, fallback opcode/reason, links created/broken,
code/meta cache high-water marks and eviction/recompile causes. Count emitted
instructions/spills/helper sequences at compile time. Those describe code
shape, **not** how often a hot linked block executes. Use bounded optional
execution samples to answer that separate question.

Start with two repeats of the four-scene `subsystem_gaps` chain (SM64, MK64,
Snap, DK64), then a larger CPU-stressing chain including Zelda and PAL MP3.
Use existing recorded gameplay where available; add fixed routes before
generalizing title-screen results. Startup and steady-state get separate rows.
Large ROMs exercise paging; FPU-heavy, interrupt-heavy and SMC cases need
focused tests as well as games. Preserve ROM region, checksum and settings.

Compare counters-only, existing probes, and CPU-focused probes on one source
revision and agent version. Test multiple sampling intervals and offsets to
detect aliasing; use jitter only if repeatability remains recorded. An initial
CPU-focused overhead target is <=3% added cycles, not a measured guarantee.
If exceeded, reduce scope/sampling and keep instrumentation-only runs separate.
Measure agent/heartbeat cost separately in controlled development variants;
ordinary Wii releases now include it. Keep a probe-off release smoke test;
it cannot provide the same detailed counters.

### 3–6. Optimization loop

For each candidate: reproduce, measure, hypothesize, change one thing, run its
focused correctness check, test Dolphin, then repeat matched Wii A/B/A. Reverse
the order or run more repeats when variation is as large as the claimed gain.
Keep probe boundaries identical between candidate and reference. Report cycles
per VI, non-sleep wall time, speed, frame-time distribution when available,
underruns/overruns, cache pressure and startup cost. Do not accept a healthy
average that conceals crashes, black frames, longer stalls or worse audio.

Dispatch work begins with the current cached-target and compiled-JR paths, not
an assumed need to replace the tree or reimplement last week's optimization.
Audit stale-table handling on TLB writes, invalidation, compile-in-place and
freeing functions, including short stores and aliases. Count slow-path misses
and distinguish C dispatches from direct generated-code jumps.
Test overlapping function ranges, insert/remove, stale links, cache-full retry
loops, metadata exhaustion and repeated eviction. Keep guest aliases and
self-modifying-code invalidation correct. Check D/I-cache synchronization after
patching or publishing generated code. Measure before moving hot metadata or
code between MEM1/MEM2; spare bytes do not establish a faster placement.

Inspect generated PPC with the matching ELF and bounded block dumps. Preserve
LR/CTR/CR/XER, stack alignment and helper ABI, upper halves of guest values,
FPR mappings and rounding. Separate compile-time register-cache work from
runtime spills/reloads. Use available Broadway PMCs only after verifying event
encodings and reporting their configuration. Dolphin is not a Wii PMC oracle.

Idle-loop shortcuts must preserve Count/Compare, wakeup conditions and event
ordering. Do not disable interrupts or alter `noCheckInterrupt` merely to make
a timing smaller. Do not remove interpreter fallbacks without instruction tests.

### 7. Completion

Reuse the library validator and queue runner. Keep new and old scene captures,
hashes, build flags, region, replay and failed-run reasons. Check both renderers,
ROM switching, cold/warm startup and long sessions. A completed VI target alone
does not establish correct video or sound. Timed chains must not overwrite saves.

Advance only with a stable improvement outside measured variation, no new
correctness failures and acceptable probe cost. Publish a per-game comparison,
known limits and deferred hypotheses. Bump the version for code changes,
commit and push the tested changes to `master` on the fork.

## Local code-graph setup

Use [codebase-memory-mcp](https://github.com/DeusData/codebase-memory-mcp/releases/tag/v0.11.0),
pinned to 0.11.0. It has native Intel macOS and Windows builds and C/C++ parsers.
No Docker, embedding service or new language runtime is needed. The tested Mac
archive is about 40 MiB; its unpacked executable is about 287 MiB. Cache data
stays in ignored `.dev/codegraph-cache`. The older `.codebase-memory` artifact
is retained but is not evidence that the current source was indexed.

Download the platform archive and `checksums.txt` from that release. Verify the
archive's SHA-256 against the exact filename before extraction. Put the binary
in `.dev/tools/codegraph-v0.11.0/` or set `WII64_CODE_GRAPH` to an absolute path.
Windows uses the release's `windows-amd64.zip` and the devkitPro MSYS2 Bash
launcher; this Windows setup has not been tested on this Mac.

On this Intel Mac the archive SHA-256 is
`dbf1c73bfcbde64e7dde4cd1320da7afc02e2c972ee1789ae039521411f5132e`.
No upstream auto-installer was run: it can alter unrelated agents' configuration.

```bash
bash .dev/code_graph.sh config set watcher_enabled false
bash .dev/code_graph.sh config set auto_watch false
bash .dev/code_graph.sh cli index_repository --repo-path "$PWD" --name wii64 --mode fast
python3 tests/code_graph_test.py
bash .dev/code_graph.sh cli trace_path --project wii64 --function-name dynarec --depth 1 --limit 30
```

Run indexing between test jobs, not during benchmarks. Refresh after source
changes. Check `index_status` and `check_index_coverage` before relying on a
query; inspect parser errors in the specific file. After rebasing, the refreshed
index recorded 33,345 nodes and 85,556 edges; 77 files had partial parses.
`Rice_GX/TextureFilters_hq4x.h` and `main/perf_subsystem.c` had unusable parses;
read those files directly, particularly the probe definitions. These counts
are not coverage percentages. C/C++ call edges can be heuristic; macros, function pointers,
conditional compilation, assembly and generated PPC require source validation.
An edge to a `PROFILE` function is not proof that the release executes it.

This Mac is registered with `codex mcp add wii64_code_graph -- /bin/bash
/absolute/path/to/Wii64/.dev/code_graph.sh`. The name is global, but the launcher
always uses this repository and its private graph cache. Other agents were not
reconfigured. Default server startup uses the upstream read-only `analysis`
profile; indexing remains an explicit CLI action between benchmark jobs.

Alternatively, for [project-local Codex MCP registration](https://learn.chatgpt.com/docs/extend/mcp?surface=cli),
use this ignored `.codex/config.toml`, with absolute paths for your checkout:

```toml
[mcp_servers.wii64_code_graph]
command = "/bin/bash"
args = ["/absolute/path/to/Wii64/.dev/code_graph.sh"]
cwd = "/absolute/path/to/Wii64"
startup_timeout_sec = 30
tool_timeout_sec = 60
enabled_tools = ["index_status", "check_index_coverage", "list_projects", "get_graph_schema", "get_architecture", "get_file_outline", "get_code_snippet", "search_graph", "search_code", "query_graph", "trace_path", "detect_changes"]
```

Project configuration requires a trusted project. Restart the MCP connection to
load new tools, then confirm the server is connected. `codex mcp list` confirms
configuration, not a successful handshake; the smoke test checks initialization,
tool discovery and actual CPU call queries. UI is disabled by the launcher.
Deletion, indexing and ADR-writing tools are absent from default server startup.
The registered configuration is verified, but this chat's existing tool catalog
has not reloaded it. The actual stdio handshake and CPU queries passed through
the smoke test. Targeted coverage checks flagged partial parses in `Wrappers.c`
and `MIPS-to-PPC.c`; the six checked CPU/memory files matched indexed metadata.

The Wii's central lease was held by another project's job during setup. No Wii
requests were sent and no lease was taken. After rebasing, the current-source
subsystem suite and ROM/VM layout suite passed, including the 8 KiB layout.
The clean glN64 build and muted MMU/LLE Dolphin startup check passed **before**
rebasing, against the older CPU/agent source. They do not validate the rebased
runtime. Rebuilding that runtime on this Mac first requires a newer external
HBC-Reborn checkout and a matching local SDK archive. No new ROM performance
baseline or hardware recovery test was run in this session.

## Resume point

Update 2026-10-03: [the targeted optimization audit](optimization-audit-2026-10-03.md)
fixes written-byte invalidation coverage and two Mac build/queue blockers.
The isolated current SDK builds and layout tests pass; shared-source/client
refresh and real-Wii recovery validation remain pending. A current-runtime
full/control/full survey is queued; collect it before choosing another hot path.

Update 2026-10-04: [hardware validation](hardware-validation-2026-10-04.md)
collects that survey and passes the SDK 1.9.4 VM-active DSI recovery test.
Dispatch is much smaller; graphics is a stable observed cost in Kart and Snap.
Execute/helper estimates still alias, and full probes add 12.9–16.2% cycles.
Do not select another CPU speed change from those estimates.

The user's WiiStation comparison now selects a [hardware PC sampler and
targeted probe tiers](performance-methods-2026-10-04.md) as phase 2's next
measurement experiment. This design is not yet implemented or calibrated.
Retain light controls; sample shares do not replace wall, PMC or GP metrics.

Finish phase 0's default shared-source/client refresh and recovery validation
before new hardware CPU changes; preserve the integration already merged into
the fork. In parallel, complete phase 1's source map and comparison-harness
audit. Phase 2
must rerank the CPU after the existing dispatch/JR changes. In phase 3, audit
those paths before choosing any additional speed change. The isolated SDK
1.9.4 build has passed VM-active DSI recovery on the Wii; do not treat that as
completion of the separate fatal/hang/retained-output checks.

Local review used Muse Glimmer and Gemma4 concurrently with 10,000-token caps
and 300-second timeouts; both answered in about 78 seconds wall time. Suggestions
were checked against source. Unsupported overhead guarantees, arbitrary cache
thresholds, and treating the interpreter as an infallible reference were rejected.
A second review took about 48 seconds. It prompted visible smoke-test stderr;
its incorrect hexadecimal arithmetic and MCP-sequence claims were rejected.
