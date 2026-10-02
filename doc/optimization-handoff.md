# Optimization handoff: 1.6.2

## Audio status

Keep Accurate / Wii DSP / Accurate / Stable / Native Rate as the reference.
The compatibility cleanup, diagnostics, settings, and tested synthesis/output
paths are complete. Hi-Fi and Preserve Pitch remain experimental: the combined
enhancements cost about 22–25% more CPU cycles in the tested Wii scenes and
can overrun. They are not custom DSP synthesis. See [Audio settings](audio-settings.md)
for measurements and [the coverage matrix](audio-development-plan.md#roms-needed-for-microcode-coverage)
for untested microcode paths. Separate voice routing, wide accumulation,
custom DSP firmware, and optional post-mix reverb are future projects.

Run `.dev/test_rsp_audio.sh` before changing audio arithmetic or output handling.
It checks reference PCM/state under sanitizers, streaming and pitch invariants,
queue boundaries, configs, reports, captures, and log handling. Preserve matched
ROM/scene/input, VI count, DOL/ELF, toolchain, and probe flags for comparisons.
`scripts/chain_compare.py A B` reports cycles, speed, queue samples, underruns,
and overruns. Its `queue_peak_ms` is the sampled game summary; the producer
duration peak is reported separately by `scripts/audio_report.py`.

## Next candidate: graphics display-list translation (2026-10-02)

The [GPU survey](gpu-results-2026-10-02.md) measured the GP's own counters on
the Wii: the GP is idle more than 85% of the time in nine 3D scenes. Graphics
cost is CPU-side glN64 work. Mario Kart's `SETCIMG` billboard copy is now twice as fast
(race 0.978x to 0.993x), vertex loads use the paired-single unit (−2 to −3.5%
cycles), the dynarec dispatcher remembers its targets (−1 to −6.6%), and Kart skips
half its billboard copies (race −18.8% more). An 18-ROM library check passed.
In a second round, compiled `JR`/`JALR` jump through the dispatcher's table
(82–95% fewer returns to C), DMA invalidation makes one tree query per freed
func (together −5.1 to −9.0% CPU cycles), and the envelope mixers cost
30–40% less per sample. What is left, largest first: compiled code itself, the
display-list translation in Snap, Smash, Wave Race and Kart, and the slow
memory handlers (TLB-mapped accesses in GoldenEye). See
[the changes and rejected experiments](gpu-results-2026-10-02.md#changes-from-this-survey-2026-10-02). Paging is now
on by default; the section below is its history.

## Earlier candidate: large-ROM paging

The [six-game subsystem survey](subsystem-profile-2026-09-30.md) found 2.6–3.3
seconds inside ROM copies for three 32 MB titles, versus 14–25 ms for the 8–16 MB
ROMs. Those larger ROMs exceed the 16 MB cache and use the NAND-backed VM path.
Count and time page-ins and evictions before changing VM behavior. Actual limiter
sleep matched requested sleep within 3 ms per scene, so oversleep does not explain
the observed seconds-scale stalls. Pokémon Snap's graphics span is a separate
candidate at 36.7% of Wii wall time. These startup scenes are not whole-game results.

Use `.dev/profile_subsystems.sh` for frozen full/control/full builds through the
central Wii lease. New timers stay opt-in. Inclusive sparse timings are estimates,
not additive CPU utilization; preserve the matching binaries and configurations.

## Secondary candidate: dynarec function lookup

`r4300/ppc/FuncTree.c` uses an unbalanced address-range tree. Existing probes
observed depth 25 in TWINE and 31 in Banjo-Kazooie. Depth alone does not establish
CPU cost. The new sparse timings estimated 1.6–4.8% of wall time in MP3/SM64/Banjo,
with lower shares in TWINE/DK64. Confirm with another interval before choosing
a change. Compare full/disabled/full probes on the same Wii scene.

1. Profile `find_func` separately from compilation and cache eviction. Freeze
   audio settings at the reference above. Do not mix Hi-Fi cost into this result.
2. Inspect both uses of `PowerPC_func_node`: page function lookup and outgoing
   links. Check overlap handling in `Recompile.c` and invalidation/unlinking in
   `Recomp-Cache-Heap.c`. Do not replace both trees without a measured need.
3. Choose the smallest change that reduces measured cost. Test lookup, ordered
   insertion, removal, overlapping ranges, eviction, and stale-link lifetimes
   against a reference before testing a DOL.
4. Run matched multi-game Dolphin checks, then Wii A/B/A runs through the shared
   queue. Compare cycles, speed, exceptions, recompiles, tree depth, and audio
   underruns/overruns. Exclude Zelda from the test chain as requested.

Heap construction already uses bottom-up Floyd heapify; it is not pending work.
Keep the Mario Kart black-screen/boot issue separate from performance results:
a stalled scene with no synthesized audio is not an audio benchmark.
Also fix the magenta diagnostic XFB captures in Dolphin before using those
images to confirm gameplay. Wii captures currently provide scene confirmation.

## Workstation storage

An inactive `.dev/dolphin_mario_triage/Logs/dolphin.log` occupied about 21 GiB.
Lossless `gzip -1` reduced it to 951 MiB; `gzip -t` verified the archive.
Free space rose from 6.3 to 24 GiB during cleanup. No ROMs, saves, baselines,
or matching debug binaries were deleted. Other applications' caches were left
alone. The archive remains under the same profile as `dolphin.log.gz`.

Timed `.dev/dolphin_test.sh` runs now check log size before boot and every five
seconds. The default threshold is 64 MiB; set `WII64_DOLPHIN_MAX_LOG_MIB` to a
positive integer to change it. A run exceeding the threshold stops its own
profile's Dolphin, retains diagnostics, and returns a failure. Output can exceed
the threshold between checks and during shutdown; this is not a strict disk cap.
Interactive launches have only the preflight check. Fault logging is not disabled.
The final boot-log tail now retains only 20 lines in memory.

Reuse isolated test profiles instead of creating one for each run. Keep frame
dumping opt-in and retain captures only when they answer a test question. Do not
remove WiiSD images or profile sync folders without checking for unsynced saves.
Before compressing an old log, confirm its profile is stopped. To recover the
large archive, use `gzip -d` on that exact file with at least 21 GiB free.
