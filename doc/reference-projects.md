# Reference projects for future compatibility/perf work

Catalog of external N64-emulator codebases worth mining for fixes, features, or
optimizations to bring into this project. Compiled while investigating the
CIC-6105 boot hang (Majora's Mask / Zelda OoT Master Quest never complete
boot) -- see `doc/subsystem-review.md` section 7/8 for that investigation.
None of these are checked into this repo or cloned persistently anywhere;
clone fresh (shallow is usually enough) when you actually need to read one.

| Project | URL | What it's good for | Status here |
|---|---|---|---|
| **Not64** | https://github.com/extremscorner/not64 | PPC dynarec, fastmem, memory/cache behavior, Wii-specific optimizations. Actively maintained (Extrems), same lineage as this repo (shared tehpola/sepp256/emu_kidid ancestry). | Compared PIF/CIC-6105 code: byte-identical to this repo's. No fix found there for the boot hang. Has one thing we don't (nothing CIC-relevant); we have several things it doesn't (mempak bounds check, `pif_reset_state()`, SI-DMA-direction tracking). Its `write_pifd()` has the same double-write bug this repo already fixed. Worth a deeper look for **dynarec/fastmem-specific** work (its actual specialty) in a future pass -- not yet done. |
| **Wii64 1.4.9** (upstream) | https://github.com/emukidid/Wii64 (tag `1.4.9`) | The direct ancestor this repo forked from. Best single diff target for "did we regress something" or "did upstream already fix X". | Diffed fully against this repo's `pif.c`, `n64_cic_nus_6105.c`, `cicx105.c`, `exception.c`, `memory.c`. CIC-6105 algorithm identical. This repo is strictly ahead (MI-mirror fix, `write_pifd` fix, systemic MMIO bounds checks) -- nothing upstream was missed. Confirms the boot hang is NOT a regression from upstream; it's a bug present in the shared ancestor too. |
| **Mupen64Plus Core** | https://github.com/mupen64plus/mupen64plus-core | CPU correctness, interrupts, timing, RSP/HLE fixes. Different lineage, same underlying N64 hardware behavior -- good as a "what does correct look like" reference. | **Most useful lead found so far.** Its PIF status-byte (`0x3F`) dispatch (`process_pif_ram()`) treats the byte as an OR-able bitmask and always resolves to a terminating state; this repo's `update_pif_write()`/`cic_challenge` handling is a sticky flag that isn't cleared for anything other than the literal challenge case, and can leave `update_pif_read()` permanently no-op'ing. See "Current best lead" below. |
| **GLideN64** | https://github.com/gonetz/GLideN64 | RDP/RSP knowledge, microcodes, framebuffer algorithms. | Checked for CIC/boot-hang relevance: none, as expected for a pure graphics plugin (confirmed zero CIC/ucode-boot-detection code in the whole repo). Its Zelda-specific hacks (`hack_ZeldaMM`, `hack_subscreen`, `hack_ZeldaMonochrome`) are post-boot rendering quirks only. Still a good future reference for RDP/framebuffer work generically. |
| **mupen64gc-FIX94** | https://github.com/FIX94/mupen64gc-fix94 | Game-specific Wii/GX fixes and boot/compatibility hacks. | Checked for CIC-6105 relevance: dead end. Its PIF/CIC subsystem is architecturally frozen at a pre-`n64_cic_nus_6105.c` snapshot (uses an old hardcoded challenge/response lookup table, `pif2_lut`, instead of the algorithmic approach this repo and upstream Wii64 both use) -- nothing to port for this specific bug. Its other game-specific hacks (DK64/Blast Corps DP-freeze handling, a few GX fixes) could still be worth a scan for unrelated compatibility issues later. |
| **Rice GX** | (not standalone) | Fast alternative rendering paths, game hacks. | Lives inside Wii64/Not64 itself (`Rice_GX/`), no separate upstream repo. This repo already has it (`Rice_GX/`). |
| **libogc2** | local: `C:\projects\libogc2-src` | Wii/GC hardware, memory, I/O, runtime improvements. | Already local, already indexed -- see the `wii-homebrew` skill. |
| **modern PPC compiler work** | (no specific repo identified yet) | LTO, code layout, register pressure, generated PPC tuning. | Not researched yet -- "modern PPC compiler work" wasn't a concrete project name given, just a topic. Worth a dedicated search (e.g. recent devkitPPC/GCC PPC backend changes, or other Wii homebrew projects' build-flag tuning) if pursued later. |
| **Project64** | https://github.com/project64/project64 | Mature, historically excellent CIC-type compatibility. | **Second independent confirmation of the same lead as mupen64plus-core.** Its PIF control-byte handling is fully *stateless* (`PifRamHandler::ControlRead()` checks `m_PifRam[0x3F] == 0x2` fresh every time) -- no persistent "are we mid-challenge" flag at all. Also implements `0x10`/`0x30`/`0xC0` control-byte cases this repo doesn't. |

## Current best lead for the CIC-6105 hang (not yet applied)

Two independent agents (comparing against mupen64plus-core and Project64
separately) converged on the same spot: `gc_memory/pif.c`'s `cic_challenge`
flag.

```c
// update_pif_write(), gc_memory/pif.c
if (PIF_RAMb[0x3F] > 1)
{
    switch (PIF_RAMb[0x3F])
    {
        case 0x02: /* ...compute challenge response... */ PIF_RAMb[0x3F] = 0; break;
        case 0x08: PIF_RAMb[0x3F] = 0; break;
    }
    cic_challenge = 1;   // set for ANY control byte > 1, not just the real challenge (0x02)
    return;
}
```
```c
// update_pif_read()
if (cic_challenge) return;   // sticky -- only cleared by a later *full* normal command loop
```

Both PJ64 and mupen64plus-core instead gate on the *current* value of
`PIF_RAMb[0x3F]` at read time (stateless), not a persistent flag set by
whatever the last write happened to be. If CIC-6105 IPL3 issues a `0x3F`
write with any value other than exactly `0x02` in the middle of its boot
sequence (e.g. `0x08`, or a value this repo doesn't handle like `0x10`/`0x30`
which PJ64 does), `cic_challenge` gets set and every subsequent
`update_pif_read()` silently no-ops until a full plain command block happens
to run -- which may never happen if the guest is itself waiting on data from
that stalled read. Matches the observed symptom: dynarec_trace showed genuine
execution progress (768+ real dispatches, not the old "PC oscillates to 0"
bug) that then froze in place, with a sustained ~5x-normal guest exception
rate, consistent with a spin-wait on PIF status that our emulation never
resolves.

**Update: implemented and tested -- did NOT fix the Majora's Mask hang.**
Applied as a real, independently-justified correctness fix (commit
`2492df2`, `gc_memory/pif.c`) since it matches two mature reference
implementations and Banjo-Kazooie still boots/plays fine with it. But
`dynarec_trace=1` against Majora's Mask shows an *identical* freeze
before and after the change -- same dispatch count (#768), same PC
(`0x8009552C`), same preceding exception at dispatch #754
(`PC=0x80000180`, the general exception vector). So this PIF-read gating
bug is real but not what's actually stalling this ROM's boot; the actual
hang is downstream of it.

**Further correction, next session**: the "dispatch count frozen at
#754/#8009552C" reading above turned out to be a `dynarec_trace` artifact,
not a real freeze -- direct instrumentation showed the dynarec's outer C
dispatch loop genuinely running (millions of real dispatches, not stuck)
the whole time. See `doc/subsystem-review.md` section 8's "Correction to
the 'linked-block native loop' theory" entry for the full, corrected
picture and concrete next steps. Short version: this ROM dispatches
millions of blocks without ever completing boot -- real, varied execution,
not a frozen or simply-cycling loop -- and no fix has landed yet.
