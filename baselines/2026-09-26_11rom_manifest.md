# Eleven-ROM baseline manifest

Build: `ae4ad95` with `PERF_PROF`; all observed Dolphin runs used dynarec. Each entry in `scripts/chains/bench_non_zelda.txt` runs for 3,600 guest VIs. The six Mario Party entries use `press_a_periodically` (11 loaded records). The two Zelda files are excluded because they are known to fail on this build.

SHA-256 of the chain: `8b913079b3b93597e3741539ce8a75bb3ed316b15dedbf9babb9910b62ba2b4c`

SHA-256 of the input replay: `5a952389e4e4700fe4818593c2c7a09cdca3de2f35f7c2dd3e07c3e22a1f4e3c`

| ROM file | SHA-256 |
|---|---|
| Super Mario 64.v64 | `f3b0fc9f565f995368b6fa5aefc8ec1e5194c488948b41f3534dece4abcec2b4` |
| Mario Kart 64.v64 | `9d538ee9dc7a61d9794c2f643775677bc844a3f297327f8a9ae36bde0c51f431` |
| Banjo-Kazooie.V64 | `43f9866ab70d15a19e7b4c7a3f133d18140ecacff6135d1e9946d55e9beb7a90` |
| Mario Party.v64 | `b946ff34082c92e297ce14689a08c7ac10afa1852a23fba9d2b7d3ab1b926f6e` |
| Mario Party (USA).z64 | `ca4fb9605fff4884e9ba4319dfa23d96b7347ce88ffb8d04e6c25a3a9ff9ed6a` |
| Mario Party 2 (E) (M5) [!].z64 | `4fe18c71dba3520dab2a0618fdd949cf0869579325ea2ecfd2040337daff3863` |
| Mario Party 2 (USA).z64 | `0b7b2ec3bd2ac8713b4c43f74a634285a720779964ee2658f7ad2dfa97b33576` |
| Mario Party 3 (E) (M4) [!].z64 | `23a33bd5ec1ef62f6888e0d7b68f56ffb36da83025be789ca1170da43266242f` |
| Mario Party 3 (USA).z64 | `a08cbd6a4f40d15cbd8bcdee644f80cdfb843e06d569d1334bcd49f23262855a` |
| Pokemon Snap.rom | `a1d5d816db7f8557ee04c35a011326d058b2c1fbca76b57b352b1d705a1ec1cc` |
| Super Smash Bros. (U) [!].z64 | `15592e79d3c5295cef4371d4992f0bd25bec2102fc29644c93e682f7ea99ef3d` |

Profiling DOL SHA-256: glN64 `82a8e2d33f4b2ca5f0202328ca29dda8bad0a7b451f54557d0f53539ba28acae`; Rice `44b35258adf104bbaee89ebbc98677f76e720c03a2723863537910da7319b71b`.

Use the same ROM hashes, chain, input replay, plugin, and platform when comparing a later run. Do not compare Dolphin `idle_pct` or IPC with Wii hardware values.

The 11-ROM Dolphin baselines are `2026-09-26_dolphin_glN64_11rom` and `2026-09-26_dolphin_Rice_11rom`. The Wii card still needs the three USA Mario Party copies before the same 11-ROM chain can run there. `2026-09-26_hw_glN64_9entry` used the existing eight-ROM `hardware.txt` chain, with no Mario Party replay and an extra Super Mario 64 stick check; compare it with the older nine-entry hardware baseline, not directly with this 11-ROM chain. Dolphin's raw XFB captures can be solid color even when live video renders correctly; use the Wii captures for the visual check.
