# Wii64/glN64 optimization baseline

Reconnaissance date: 2026-09-14. This document records the source and build baseline before behavioral or performance changes.

## Source baseline

| Item | Exact reference |
|---|---|
| Upstream | [emukidid/Wii64](https://github.com/emukidid/Wii64) |
| Latest stable tag | `1.4.9` at `57e34e7` (`- Bump version`, 2026-09-13) |
| Chosen development baseline | `master` at `9eb19ee06be90ba607e207f8f63743d05cd438d4` (`- Fix texture warping/viewport near issues`, 2026-09-13) |
| Baseline rationale | Current development tip is two commits beyond `1.4.9`; the latest commit is renderer correctness work and is the right starting point for later measurements. |

The working tree is now a normal Git repository on the dedicated local `reconnaissance` branch, with the imported upstream history preserved. `origin` is configured as `git@github.com:Monsterray/Wii64.git`. No source changes were made for this session.

## Build baseline

Wii64 uses GNU Make, devkitPPC’s `powerpc-eabi-*` tools, `elf2dol`, libogc2, and static Wii/GameCube libraries. The main build families are:

| Renderer | Wii | Wii VC | GameCube | GameCube expansion |
|---|---:|---:|---:|---:|
| glN64 GX | `Makefile.glN64_wii` | `Makefile.glN64_wiivc` | `Makefile.glN64_gc` | — |
| Rice GX | `Makefile.Rice_wii` | `Makefile.Rice_wiivc` | `Makefile.Rice_gc` | `Makefile.Rice_gc_exp` |

Common flags in `Makefile.base` include `-O3`, LTO (`-flto -fno-strict-aliasing`), `-mcpu=750`, hard-float EABI, `-DGEKKO`, `-D__GX__`, `-DPPC_DYNAREC`, `-DUSE_RECOMP_CACHE`, `-DFASTMEM`, and `-DNO_ASM`. Profiling is opt-in through `DEBUG_FLAGS` (`-DPROFILE` is currently commented out).

## Build result in this environment

The glN64 Wii target was invoked as a host preflight. It stopped before compilation because `/bin/powerpc-eabi-g++` is unavailable; `DEVKITPRO` and `DEVKITPPC` are also unset. This is a toolchain/environment blocker, not a source failure. There is no Wii hardware attached, so no FPS or frame-time number is claimed here.

The first real build must be performed with the project’s intended devkitPPC/libogc2 environment. A profile build and a release build must remain separate; profile output is not a performance baseline.

## Upstream/reference points

| Project | URL | Revision used for reconnaissance | How it informs this project |
|---|---|---|---|
| Not64 | [extremscorner/not64](https://github.com/extremscorner/not64) | `d35a7a1c454eca973995939dbca3f06725cf4a6e` | Closest active Wii/GC fork; useful for isolated PPC, VM, framebuffer, and libogc-rice comparisons. |
| FIX94 fork | [FIX94/mupen64gc-fix94](https://github.com/FIX94/mupen64gc-fix94) | `e5d9d9cc9fea4b1abaf79635cd00893a56a03ff9` (`r2`) | Historical Wii U/GX compatibility and boot fixes; not a performance drop-in. |
| Mupen64Plus core | [mupen64plus/mupen64plus-core](https://github.com/mupen64plus/mupen64plus-core) | `cf00a1d1b4138a8e04cde47aabcd00ea9a047f57` | Portable dynarec, memory, timing, and profiling concepts; no PPC/Wii target. |
| GLideN64 | [gonetz/GLideN64](https://github.com/gonetz/GLideN64) | `41c7ba273a6c9afb43c0574cf3cf5d139182d070` | Renderer behavior/reference only; its desktop OpenGL architecture is not a full GX port plan. |
| libogc2 | [extremscorner/libogc2](https://github.com/extremscorner/libogc2) | `f34ec5d79542b55f4780b0a75be37ead5011c18c` | Current platform API dependency; Wii64 makefiles already select libogc2. |

Wii64 identifies itself as GPLv2 software. Any copied code still needs per-file license/header review and a record of the source revision. ROMs, BIOS files, save files, screenshots containing copyrighted game data, and benchmark captures with ROM data do not belong in this repository.

## Relevant optimization history already in Wii64

The codebase already contains fast memory access, PPC function linking, a register cache, a recompilation cache, TLB-cache variants, and timing work. Important historical evidence includes `d4c642a` (Not64 integration including fast memory and renderer changes), `aab0a6c`/`ede532f` (TLB-cache sizing/layout decisions), `9b02deb` (fast-memory/register-state work), `bbd878f` and `2bca1cb` (fast-memory correctness fixes), and `c66d6ee` (timing alignment). These are constraints and prior experiments, not fresh work items to re-import wholesale.
