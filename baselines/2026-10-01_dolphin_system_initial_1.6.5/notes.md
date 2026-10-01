# 2026-10-01_dolphin_system_initial_1.6.5

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 76a86b6+system-probes-1.6.5
- purpose: Eight-entry dynarec survey; title/intro and replay scenes. USA Parties on Dolphin, PAL hardware rerun separately. Mario Kart low-work scene is not healthy gameplay.
- screenshots: baselines/media/2026-10-01_dolphin_system_initial_1.6.5/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 60.5 | 28.0 | 14185 | 1572 | 14 | 9 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.00 | 1.000 | 97.9 | 2.0 | 5145 | 395 | 14 | 0 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.33 | 0.979 | 44.7 | 27.6 | 5450 | 2084 | 31 | 4 |
| 4 | Mario Party.v64 | vis | 900 | 15.06 | 0.996 | 66.9 | 57.7 | 7672 | 1420 | 25 | 6 |
| 5 | Mario Party 2 (USA).z64 | vis | 900 | 15.03 | 0.998 | 75.7 | 64.2 | 8650 | 1706 | 26 | 3 |
| 6 | Mario Party 3 (USA).z64 | vis | 900 | 15.01 | 0.999 | 77.9 | 61.4 | 7857 | 1731 | 20 | 4 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.35 | 0.977 | 12.4 | 53.9 | 10824 | 777 | 12 | 19 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.03 | 0.998 | 42.7 | 54.6 | 15999 | 856 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_dolphin_system_initial_1.6.5 <other id or run dir>`
