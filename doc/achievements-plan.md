# Achievements plan

Status: research complete; implementation not started. Checked 2026-10-10 against
Wii64 master `ba0a0a45b9ad455a86760280daf8c40939b9213a` (1.9.3).
This is a survey of relevant systems, not a claim to enumerate every service.
Recommendations and acceptance thresholds below are proposals, not measured results.

## Recommendation

Integrate **RetroAchievements**, using its existing N64 sets and the official
`rcheevos` C library. Start with opt-in Casual mode. First prove memory reads,
frame timing, and memory capacity with a mock server; then add secure networking
and a small menu. Do not create another achievement-expression parser or a
multi-provider framework.

The official support list includes Mupen64Plus-Next, ParaLLEl N64 and Project64
variants for N64, but does not list Wii64. A working integration is not the same
as approved hardcore support. [Supported emulators](https://docs.retroachievements.org/general/emulator-support-and-issues.html).

The current official library release is **v12.5.0**, commit abbreviated
`1433173`. Pin its resolved commit before building; keep its MIT notice.
The library provides evaluation and client APIs, not networking or UI.
Use its stable release rather than tracking `develop`.
[Release](https://github.com/RetroAchievements/rcheevos/releases/tag/v12.5.0),
[library README](https://raw.githubusercontent.com/RetroAchievements/rcheevos/develop/README.md),
[license](https://raw.githubusercontent.com/RetroAchievements/rcheevos/master/LICENSE).

## Systems surveyed

These suitability decisions are engineering assessments. Storefront APIs can
record achievements for registered products; they do not supply N64 RAM conditions.

| System | What it supplies | Decision for Wii64 |
|---|---|---|
| RetroAchievements | Community N64 sets, account progress, leaderboards, runtime integration | Primary choice; existing game definitions are the key advantage. [Support](https://docs.retroachievements.org/general/emulator-support-and-issues.html) |
| Local/offline achievements | Our own conditions and personal progress records on SD/USB | Optional later; no account, but definitions and maintenance become our responsibility. Not official RA unlocks. |
| Steamworks | Per-application stats and achievement APIs; desktop Steam runtime | No documented native Wii runtime or retrofit N64 sets. [API](https://partner.steamgames.com/doc/features/achievements) |
| Epic Online Services / Epic Games Store | Product-configured achievements; EGS adds store XP | Requires a product/authentication integration and our own conditions. Wii feasibility not established. [Setup](https://dev.epicgames.com/docs/epic-games-store/services/epic-achievements/achievements-setup) |
| GOG GALAXY | Product achievements and Galaxy-backed offline synchronization | Not a standalone Wii achievement database. [Workflow](https://docs.gog.com/sdk-stats-and-achievements/), [feature matrix](https://docs.gog.com/sdk-galaxy-feats-and-states/) |
| Xbox achievements / PlayStation trophies | Registered-title platform services and partner tooling | Not a public retrofit backend for Wii64. [Xbox](https://learn.microsoft.com/en-us/gaming/gdk/docs/services/player-data/achievements/live-achievements-eb-vs-tm?view=gdk-2604), [PlayStation partner access](https://sonyinteractive.com/en/news/blog/showing-your-game-to-playstation/) |
| Apple Game Center / Google Play Games | App-specific achievements on their platform ecosystems | A companion would be a separate integration, still needing N64 conditions. [Apple](https://developer.apple.com/game-center/), [Google](https://developer.android.com/games/pgs/achievements) |
| Game Jolt trophies / Newgrounds medals | Game-project APIs for submitting developer-defined awards | Potential custom-service alternatives, not a ready N64 set library. Game Jolt documentation was search-visible but direct retrieval failed; transport/security suitability remains unverified. [Game Jolt](https://ssr.gamejolt.net/help-docs/creators/game-api), [Newgrounds API](https://www.newgrounds.io/help/components/) |
| Achievement Watcher / Watcher Next / Playnite SuccessStory | Read or aggregate existing achievement records; notifications/manual tracking | Possible future desktop viewer, not a Wii64 detector. [Watcher](https://github.com/xan105/Achievement-Watcher), [Next](https://github.com/Shirowwww/Achievement-Watcher-Next), [SuccessStory](https://github.com/Lacro59/playnite-successstory-plugin) |

No reviewed alternative provides a ready N64 definition catalogue comparable to
RetroAchievements. This is a finding within the surveyed scope, not proof that
no other system exists. Online support depends on the exact ROM revision having
a linked hash and a set; title names or header CRCs are insufficient.

## Projects to learn from

| Project | Evidence and useful pattern | Limit |
|---|---|---|
| RetroArch + Mupen64Plus-Next / ParaLLEl N64 | N64 achievement integration. `rcheevos_test` invokes the client per frame; load/reset/state paths manage runtime progress. Mupen exposes RDRAM. [Integration](https://raw.githubusercontent.com/libretro/RetroArch/master/cheevos/cheevos.c), [core](https://raw.githubusercontent.com/libretro/mupen64plus-libretro-nx/develop/libretro/libretro.c) | Reference for the RAM contract, not permission to copy its frontend or assume byte-order parity. |
| RetroArch Wii | Current `Makefile.wii` sets `HAVE_CHEEVOS=1`, includes rcheevos headers and enables threads/networking. [Wii makefile](https://raw.githubusercontent.com/libretro/RetroArch/master/Makefile.wii) | Source configuration is evidence of a porting precedent, not proof of a successful current binary, secure transport, or N64 support on Wii. |
| RAProject64 / Luna's Project64 | N64 entries on RA's support list | Use for reference conditions and timing; Windows integration tooling is not the Wii runtime. [Support list](https://docs.retroachievements.org/general/emulator-support-and-issues.html) |
| Dolphin | `AchievementManager::DoFrame` runs on the emulated CPU thread; pause policy is explicit. [Source](https://raw.githubusercontent.com/dolphin-emu/dolphin/master/Source/Core/Core/AchievementManager.cpp) | Dolphin observes the outer Wii/GC program. It does not automatically identify the inner N64 ROM in Wii64. |
| PPSSPP | Controller-oriented account settings, mode controls and leaderboard UI. Its guide warns that evaluation timing can differ across frontends. [Guide](https://www.ppsspp.org/docs/reference/retro-achievements/) | PSP memory and UI code are not directly portable. |
| DuckStation / PCSX2 | Existing standalone RA integrations listed by RA | Useful UI/lifecycle comparisons; not N64 memory references. [Support list](https://docs.retroachievements.org/general/emulator-support-and-issues.html) |
| RA2Snes | rcheevos with QUsb2Snes-based memory access. [Project](https://github.com/Factor-64/RA2Snes) | Hardware/host-dependent reference, not a Wii64 backend. |
| odelot/wii-ra-adapter | Early ESP32-S3 EXI adapter: watched RAM snapshots, pointer-chain descriptors, sequence numbers, evaluation and HTTPS on the adapter. WiiFlow/d2x/Nintendont forks feed it. [Project](https://github.com/odelot/wii-ra-adapter) | Targets Wii/GC games, not Wii64's N64 RAM. Requires extra hardware and a new Wii64 bridge; official approval/performance not verified. Repository is GPL-3.0; do not copy into Wii64 without checking license compatibility. No cIOS installation is proposed. |

Record exact upstream commit/path and notices for any code actually reused.
No external implementation was copied during this research.

## Current Wii64 integration points

Graph searches located these paths, but coverage reports metadata changes and a
partial parse of `main_gc-menu2.cpp`. The following claims use current source,
not old graph line numbers:

- `r4300/interupt.c`, `VI_INT`, calls `new_vi()` even when presentation is deferred.
  `main/timers.c` returns if emulation is stopped. This is the candidate frame
  evaluation point, not a Wii hardware retrace callback or GX present hook.
- `gc_memory/memory.c` owns resident `rdram`/`rdramb`; `USE_EXPANSION` determines
  8 MiB versus 4 MiB. `memory.h` sets `S8=0` on big-endian hosts. Raw native PPC
  bytes cannot be assumed to match existing N64 achievement expressions.
- `main/rom_gc.c` normalizes ROM byte order. `main/ROM-Cache.c` has a 16 MiB cache
  with paging for larger ROMs; do not allocate a second whole-ROM image for hashing.
- `loadROM_unheld` in `main/main_gc-menu2.cpp` tears down the prior game before
  loading another. `main/savestates_gc.c` includes queued and direct state loads.
  Hook common operations, not only menu buttons.
- `main/dev_agent.c` already has `devAgent_netReady`; the SDK requires waiting for
  agent network initialization before another `net_init`. That helper can wait
  five seconds: never invoke it from each frame. Reuse startup coordination from
  a worker, without changing the existing HBC listener or queue protocol.
- [Memory results](memory-results-2026-10-04.md) measured 550,848 bytes of loaded
  unallocated Arena2 in the post-fix short scene. Later Transfer Pak allocations
  and longer sessions invalidate treating that value as spare capacity.
  Preserve the MEM2 map and the 8 KiB HBC record gap.

## Proposed implementation phases

### 1. Small offline feasibility build

Pin rcheevos v12.5.0, compile only the necessary runtime/client/N64 hashing code
with Wii64's existing devkitPPC/libogc2 setup, and keep dependency build outputs
ignored. No emulation semantics, UI, or production network changes yet.
Use synthetic RAM and mock service responses; use spectator mode for tests that
load real sets. Tests must never submit awards to a personal account.

Test the memory callback against the supported N64 reference:

- The pinned N64 map exposes logical offsets `0x000000..0x7FFFFF`, with the
  expansion region beginning at `0x400000`. Its `real_address` fields describe
  N64 aliases, not permission to dereference Wii addresses.
  [Pinned map](https://raw.githubusercontent.com/RetroAchievements/rcheevos/v12.5.0/src/rcheevos/consoleinfo.c).
- Mupen64Plus-Next exposes native RDRAM and `rc_libretro_memory_read` copies it
  directly. Derive a byte-order adapter from reference fixtures; test whether
  an `address ^ 3` byte mapping is needed on PPC. Do not change global RAM layout.
  [Core](https://raw.githubusercontent.com/libretro/mupen64plus-libretro-nx/develop/libretro/libretro.c),
  [memory reader](https://raw.githubusercontent.com/RetroAchievements/rcheevos/v12.5.0/src/rc_libretro.c).
- Cover byte, halfword, word, float, unaligned and cross-word reads; expansion
  bounds, missing RAM, indirect operands and integer overflow. Return the actual
  bytes-read count; use subtraction-based bounds checks. Never turn an invalid
  address into valid RAM by indiscriminate masking.

**Gate:** host fixtures and a native PPC diagnostic agree on values/events;
the linked footprint, runtime allocations and stack high-water marks are known.
Cross-compilation alone is not a pass.

### 2. Identification, frame processing and lifecycle

Use the library's N64 hash algorithm and file callbacks. It streams through a
64 KiB scratch buffer and normalizes `.v64`/`.n64` to `.z64` for hashing.
Check raw, swapped and paged ROM paths against the same known hash; test truncated
files and read failures. Do not substitute the existing ROM header CRC.
[Pinned hasher](https://raw.githubusercontent.com/RetroAchievements/rcheevos/v12.5.0/src/rhash/hash_rom.c),
[identification rules](https://docs.retroachievements.org/developer-docs/game-identification.html).

Keep one owner for client state: the emulation thread while running, then the
menu thread after `go()` returns. Evaluate once per guest VI at a validated point,
including skipped frames. No SD, network waits, GX operations, or full-RAM copies
in that hook. While paused, service client idle work without evaluating gameplay.
[Client integration](https://github.com/RetroAchievements/rcheevos/wiki/rc_client-integration).

Before first gameplay, show identification/loading progress and whether
achievements are active. Allow an explicit “play without achievements” choice
on failure; never silently activate late after missing a required title-screen
condition. On ROM change cancel stale work, drain owned callbacks safely, unload
the client game, and clear UI snapshots before RAM teardown. On reset notify the
runtime. For Casual state loads, use supported progress serialization or a
documented conservative reset for old states; never retain the previous game's
runtime progress. Do not change the existing save-state format without versioning.

**Gate:** repeatable event traces for reset, pause/resume, skipped rendering,
PAL/NTSC, dynarec/interpreter and game A/B/A; no submissions or leaked allocations.

### 3. Secure account and service transport

The emulator must provide HTTP transport. Use asynchronous requests, a unique
versioned Wii64 user agent and the login token returned by the client, not a
website Web API key. Do not store passwords. [Client API](https://raw.githubusercontent.com/RetroAchievements/rcheevos/develop/include/rc_client.h).

First audit available Wii TLS implementations/portlibs for certificate and
hostname verification, SNI, current CA roots, usable clock, code/data size and
license. The HBC channel's `http.c` accepts `http://` and port 80; it is not a
verified TLS implementation or an SDK client transport. Do not reuse it as one.

Prefer direct outbound HTTPS if it passes the capacity test. If not, separately
design an opt-in authenticated, encrypted LAN relay using an existing maintained
transport. Do not silently downgrade to HTTP, disable certificate verification,
require a workstation for all users, or stream all 8 MiB of RDRAM per frame.
Treat relay/offload as an explicit product tradeoff, not the default first build.

Use a bounded worker/request queue with timeouts, byte limits and documented
backpressure. Copy request data whose lifetime ends when a callback returns.
Deliver completions on the owner thread; game-generation IDs distinguish stale
responses, but do not by themselves solve callback-data lifetime. Complete or
cancel requests according to the pinned library contract before destroying it.

Credentials are opt-in remembered tokens, separate from ordinary settings,
excluded from logs, crash bundles, exports and Git. SD is not secure storage:
warn the user, support logout/delete, and offer session-only login. Cache only
bounded data with source/version/account identity; badges and descriptions need
their own redistribution/cache policy, not just the runtime's MIT notice.

For a connection lost after a session starts, use library retry behavior and show
pending versus confirmed awards. Cold offline/local unlocks remain explicitly
local; do not promise later official synchronization or implement a durable award
queue without verifying service policy and crash/restart semantics.

**Gate:** mock faults and real Wii login/load tests cover disconnected Wi-Fi,
DNS failure, expired tokens, invalid certificates, timeouts, oversized responses,
ROM changes in flight and shutdown. Test official unlocks only with an authorized
test account/manual action, not automated replay on a personal account.

### 4. User-facing Casual release

Reuse settings, the current-game menu and established drawing/text code. A menu
message box alone does not prove a safe in-game toast path: queue a copied event
for rendering, with explicit GX state restoration for both graphics plugins.
Start text-only; add bounded badge textures after performance validation.

Suggested settings: Achievements Off/On; account Sign in/Sign out; mode Casual
(Hardcore added only after phase 6); unlock popups; progress/challenge indicators;
notification sound; spoiler-hidden descriptions; cache limit/clear. Current Game
shows the achievement list, progress and service status. Expose errors such as
unsupported hash, no set, unsupported conditions and pending submission.
Keep gameplay audio running normally; optional unlock sound must not alter the
emulated audio engine. No general custom-server UI unless a relay is justified.

**Gate:** SD/USB and UI tests, both renderers, power/reset/HOME transitions,
allocation failure, no-network play, and a clean optional-feature-off build.
Wii first; vWii and GameCube are separate validation targets. Leave GameCube
network achievement support disabled until its memory budget/transport is tested.

### 5. Performance and library validation

Add evaluation/read-byte/event/network-queue/allocation counters to the existing
probe scheme, not a new telemetry server. Report evaluate time average/p95/max,
busy CPU cycles, non-sleep wall time, audio underruns and stack/heap peaks.
Compare feature off/on/off on the queued real Wii with frozen builds/replays,
separately from probe/control cost. Do not skip achievement frames to meet speed.

Use synthetic predicates first. Then confirm exact supported ROM hashes and sets
for our owned Super Mario 64, Mario Kart 64, Zelda and Mario Party files; include
an Expansion Pak title, a large set and a long loaded session with Transfer Pak.
This list is a proposed test selection, not a claim that every owned revision is
supported. Exercise one-frame events, counters, pointer movement and reset paths.
Run mock/spectator replays in Dolphin and Wii; compare event traces and guest
counters. Use the current library chains for graphics/audio regressions and
inspect captured frames with popups visible and hidden.

Proposed release gate: no mismatched events, lost gameplay, new memory errors or
repeatable audio regression. Aim for under 1% extra busy CPU time in representative
active sets, with explicit per-title/tail results; revise the scope if it fails,
not the evaluation frequency. Native feasibility/performance is currently pending.

### 6. Hardcore and richer features

Obtain RetroAchievements approval for Wii64's identity/version and supported
behavior before advertising hardcore. Restrictions cover state loading, cheats,
rewind/slowdown, debugging and recorded-input playback. Rich presence/leaderboards
cannot be disabled in hardcore. Enabling it mid-session requires a reset; the
current policy recommends but does not require default-on. The emulator or its
parent must satisfy the public-release eligibility period, and privacy/license
documentation is required. [Compliance](https://docs.retroachievements.org/general/hardcore-compliance-requirements.html).

Enforce restrictions in shared state-load, cheat, speed, replay and agent command
paths, not just menu visibility. Audit actual HBC capabilities; keep harmless
crash capture if acceptable, but block advantage-giving debugging/input injection
or make developer builds Casual/spectator only. Do not remove the agent wholesale.
Keep an approved player workflow distinct from diagnostic replays. Recheck policy
at implementation time. Add leaderboards, rich presence and optional local packs
only after the basic integration passes its gates.

## First experiment

Build one pinned rcheevos diagnostic using synthetic N64 RAM, a mock server and
known reference predicates. Verify byte order and per-VI event timing on Dolphin
and the queued Wii, measure peak memory and evaluation cost, then decide whether
the native design has enough headroom for TLS. No menu work or service submissions
are needed for this experiment.

## Review and open questions

- Native TLS implementation and worst-case capacity remain unverified.
- Final N64 byte transformation/frame boundary requires reference fixtures.
- Offline official-award persistence and hardcore approval require service input.
- Local-model review reinforced lifetime, memory and long-session gates. Its
  speculative request for a PPC memory barrier was not adopted: no concurrent RAM
  evaluator is proposed, and a barrier does not fix wrong frame semantics.
- Muse Glimmer's explicit design review used a 10,000-token cap and completed
  untruncated (43.9 s, 1,696 output tokens). Fetch-summary tools used their own
  1,200/1,400-token caps and truncated sources/output; they were navigation aids,
  not authority. Raw source checks supplied the important contracts.

This session changes documentation only. No version bump, new dependency,
build, ROM upload, account creation or achievement submission was performed.
