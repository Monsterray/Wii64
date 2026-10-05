# Cached and uncached memory: Wii64 recommendations

Scope: Wii64 `4ceaadc339bf49a6f728ec5be3ee2eaa61da56de`; libogc2
`43f26d22deaacae287c2f8d96eba00dba1ee0155`. This is research, not a new
performance result or an emulator change. The existing
[memory results](memory-results-2026-10-04.md) remain the Wii64 baseline.
The subsequent [cache experiment results](cache-results-2026-10-04.md)
record native ownership checks, bounded library coverage and probe overhead.

The updated reference is WiiXplorer-NG
[`d1d8d6111241690df3de6dffc16e10f136a3b53f`, MEMORY.md](https://github.com/Monsterray/wiixplorer-ng/blob/d1d8d6111241690df3de6dffc16e10f136a3b53f/MEMORY.md).
It retains one v0.1.5 native speed run: 144 verified operations, three
repetitions per combination. Its newer allocator validation does not make the
speed tables a new run. The local source checkout was clean when read.

## What the measurements support

| Bank | Cached CPU alias (K0) | Uncached CPU alias (K1) |
|---|---|---|
| MEM1 | `0x80000000–0x817FFFFF` | `0xC0000000–0xC17FFFFF` |
| MEM2 | `0x90000000–0x93FFFFFF` | `0xD0000000–0xD3FFFFFF` |

Each pair addresses the same physical bytes, not extra memory. These are
host PPC aliases, not an instruction to change emulated MIPS mappings.

Selected WiiXplorer medians, MiB/s:

| Operation | Cached | Uncached |
|---|---:|---:|
| MEM1 repeated 256 KiB read | 1210.837 | 71.097 |
| MEM2 repeated 256 KiB read | 1193.495 | 19.805 |
| MEM2 repeated 256 KiB write | 727.802 | 231.394 |
| Scalar MEM2 K0 source copy to K0 / K1 destination | 66.794 | 212.970 |

The last row changes only the destination alias and has separate 256 KiB
input/output buffers: a 512 KiB combined footprint. Read tests are warmed,
repeated workloads, not cold DRAM bandwidth. Uncached output won that scalar
copy, but uncached writes are not generally as fast as cached writes. No
number here predicts Wii64 FPS.

## Ownership rules

- CPU-read working data stays cached by default. K1 is a candidate only for a
  proven write-only output phase with a device consumer.
- Publish cached CPU writes before a device reads. In the pinned SDK,
  `DCStoreRange` uses `dcbst` (writes back while retaining clean lines);
  `DCFlushRange` uses `dcbf` (writes back and invalidates). Both synchronized
  variants finish through the SDK's syscall. They are not interchangeable
  where a later device writer requires cached copies to be absent.
- Before a device writes, prevent dirty CPU lines from later overwriting it.
  Wait for producer completion, then invalidate stale cached lines before CPU
  reads. Do not flush old dirty data over the newly written device result.
- Operate through K0 on complete, exclusively owned 32-byte cache lines.
  Account for neighboring bytes at partial-line edges. SDK range functions
  round outward; pointer alignment alone does not establish ownership.
- Do not use both aliases concurrently, free an alias independently, remove
  device fences, or treat `volatile` as cache synchronization. Keep MMIO
  uncached and retain JIT D-cache publication plus I-cache invalidation.

Sources: pinned SDK [system.h](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/include/ogc/system.h#L165)
and [cache_asm.S](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libogc/cache_asm.S#L96);
IBM [750CL manual](https://fail0verflow.com/media/files/ppc_750cl.pdf),
sections 3.3.3 and 3.4.2.3–3.4.2.5. Uncached access does not automatically
publish or invalidate an existing cached copy.

## Graphics and JIT paths

Wii XFBs come from cached fixed-map addresses in
[GraphicsGX.cpp](../libgui/GraphicsGX.cpp), lines 93–106, through
`video_mode_init`/`gfx_set_fb` to `VI.xfb`. The graph traces
`RDP_SetCImg -> gDPSetColorImage -> gDPUpdateColorImage -> YUYV_ToRGBA5551`.
The converter reads cached MEM2 XFB and writes cached MEM1 guest RDRAM;
neither side is a good default K1 target. GX copies use physical addresses
regardless of the caller's K0/K1 alias:
[GX_CopyDisp](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libogc/gx.c#L1975).

`gDPUpdateColorImage` has no local invalidation or producer-completion check.
Draw-sync and retrace callbacks asynchronously select/flip buffers
([VI.cpp](../glN64_GX/VI.cpp), lines 565–578). This is an ownership question,
not a proved stale-read bug. Establish the completed buffer and read extent
first. K1 alone would not repair reading the wrong or still-writing buffer.

The depth-copy path queues the copy, then at the next display list calls
`GX_DrawDone`, invalidates the cached buffer, and converts it
([DepthCopy.cpp](../glN64_GX/DepthCopy.cpp), lines 180–211 and 249–263).
Keep this CPU-read source cached.

`TextureCache_LoadMipChain` loads temporary levels, then CPU-copies them into
a final chain and frees them. The normal loader publishes/invalidates tiles
with `dcbf`; 2xSaI output uses a range flush. The final chain is published
again ([Textures.cpp](../glN64_GX/Textures.cpp), lines 1640–1733,
1781–1810 and 2156–2245). This proves intermediate publication followed by
CPU reads, not its cost or that publication can safely be removed. Count
chain calls, levels and published bytes first. A private CPU-only staging
path, or store-without-invalidate, needs a lifetime/eviction/failure audit.
Preserve padding initialization and the normal/final GPU publication path.

`FrameBuffer_CopyToRDRAM` flushes CPU-written guest data just before CPU
marker restamping ([FrameBuffer.cpp](../glN64_GX/FrameBuffer.cpp),
lines 1089–1149). Store-versus-flush could preserve useful lines while keeping
RAM publication. Measure this path first; audit later device/write-gather
writers before retaining cached lines.

JIT code already stays cached and performs D-cache flush plus I-cache
invalidation after patching ([Recompile.c](../r4300/ppc/Recompile.c),
lines 287–309). Preserve it. Also preserve `_gDPWriteRDRAM`'s aligned-middle
flush/write-gather/invalidate and cached-edge handling
([gDP.cpp](../glN64_GX/gDP.cpp), lines 1088–1141).

## Audio and IOS ownership

- **Audio cache ownership is already split at the right boundary.** Wii64's
  `gc_audio/audio.c:aesnd_callback` copies one queued ring chunk into static,
  32-byte-aligned `dsp_buffer` and gives it to `AESND_SetVoiceBuffer`
  ([audio.c:68-75, 93-108](../gc_audio/audio.c)). The ring can be reused after
  this copy; `dsp_buffer` itself must remain intact while AESND consumes the
  handed-off samples. libogc2's Wii implementation copies from that source into
  its private `stream_buffer` in `__aesndhandlerequest` and flushes the private
  DSP-facing buffer with `DCFlushRange` ([aesndlib.c:228-246, 249-273](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libaesnd/aesndlib.c)).
  Thus the existing explicit flush belongs to libogc2's DMA buffer, not
  Wii64's staging/ring buffers. The callback's “may still read this” comment
  documents the live-source constraint ([audio.c:100-108](../gc_audio/audio.c));
  the pinned AESND code establishes that consumption occurs by CPU copy into
  its internal buffer. **Inference:** a K1 alias for Wii64's staging buffer
  would trade any possible cache benefit for uncached CPU stores/loads; it
  would not eliminate AESND's explicit flush of its separate private buffer.

- **IOS owns cache invalidation for NAND/ISFS read destinations.** On a VM
  miss, `vm/wii_vm.c:vm_dsi_handler` calls `pagefile_read`; read-ahead mode
  reads an aligned window directly into `ROM_READAHEAD_LO`, then CPU-copies
  the requested page to its final destination ([pagefile.c:34-63](../vm/pagefile.c)).
  `ISFS_Read` checks 32-byte alignment and calls `IOS_Read`
  ([isfs.c:445-450](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libogc/isfs.c));
  `IOS_Read` unconditionally calls `DCInvalidateRange(buf,len)` before IOS
  writes the destination ([ipc.c:924-945](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libogc/ipc.c)).
  The reply handler also converts the returned physical address to K0 and
  invalidates the successful byte count before delivering the result
  ([ipc.c:327–353](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libogc/ipc.c#L327)).
  Read-ahead therefore does not bypass SDK cache maintenance. The window remains
  live through the following `memcpy` into the VM page; it is reused only after
  the copy returns. Cache hits copy from the same window to the page and do no
  new IOS call ([pagefile.c:40-62](../vm/pagefile.c)).

- **K1 is not a free cache-maintenance optimization here.** libogc2 defines
  K0/K1 aliases as cached/uncached virtual aliases
  ([system.h:165-171](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/include/ogc/system.h));
  its range invalidation walks 32-byte lines with `dcbi`
  ([cache_asm.S:96-108](https://github.com/Monsterray/libogc2/blob/43f26d22deaacae287c2f8d96eba00dba1ee0155/libogc/cache_asm.S)).
  Passing a K1 destination to the current ISFS/IOS path would still invoke that
  same SDK invalidation call. **Inference:** CPU copies/reads through K1 would
  bypass the data cache and may be slower; there is no source evidence that
  this changes IOS's explicit maintenance work. Likewise, on Wii,
  `main/ROM-Cache.c:ROMCache_read` copies from `ROMBase` ([ROM-Cache.c:165-170](../main/ROM-Cache.c));
  VM faults and read-ahead are handled in `pagefile.c`, not by this copy.


## Ranked experiments

| Rank | Candidate | Evidence / smallest experiment | Main risk |
|---|---|---|---|
| P1 | Verify XFB ownership before cached reads | Own an aligned test buffer; seed cached data, let GX write a distinct pattern, wait for completion, compare K0 before/after invalidation; trace buffer identity through callbacks | Do not overwrite a live XFB, add a blanket per-frame wait, or mistake Dolphin for physical cache validation |
| P2 | Avoid intermediate mipmap publication/refills | Count calls/bytes; test one private staging or store variant with exact texture bytes, failure fallbacks and mipmapped/filter scenes | Cache pollution, eviction callbacks and normal GPU publication |
| P2 | Store rather than flush one CPU-reused output | Measure framebuffer readback; test synchronized publication retaining lines after the writer audit | Later device writes could otherwise encounter cached copies |
| P2 | K1 final device-only texture output | Test K0 plus publication against scalar K1 plus sync, including reuse and every format | Existing loaders use cached-only dcbz; filtering/mip packing must not read K1; GX completion governs freeing |
| REJECT | K1 XFB, guest RDRAM, JIT or audio/read-ahead sources by default | CPU reads are frequent; SDK maintenance already exists | Slow reads or stale/overwritten data |

The bounded XFB producer/reader test and staging counters are now implemented;
see the results linked above. No sampled live-reader defect or staging traffic
was found in the tested scenes, so no production alias change is retained.
Do not revive the rejected YUYV dcbz/dcbt
variant or enable LC based on DMA-only bandwidth. LC halves normal L1 data
capacity; no useful end-to-end kernel win has been established here.

Retained code changes need host byte/lifetime tests, clean Wii builds,
Dolphin functional checks, and queued native A/B/A. Compare exact guest work,
CPU/non-sleep time, frames and audio underruns; count probe overhead separately.
ROMs, captures, benchmark logs and SDK source are not included in this note.

## Local model review

Muse Glimmer and Gemma4 reviewed the reference asynchronously with 10,000-token
budgets; Muse also reviewed mipmap staging. Source checks rejected write-speed
parity, a tenfold-misquoted cached-read rate, and treating guest RDRAM as
device-only YUYV output. The staging follow-up confused dcbst with keeping
dirty lines and suggested dropping padding initialization; neither is adopted.
Supply exact units and current ownership/caller context, and require a
counterexample/test for each claim. Model agreement is not hardware evidence.
