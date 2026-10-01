# 2026-10-01_wii_system_rice_full2

- date: 2026-10-01
- platform: hardware
- plugin: Rice
- build: 76a86b6+system-probes-1.6.5
- purpose: Eight-ROM title/replay survey, original per-call invalidation probes. All loads and HBC returns validated; not full gameplay coverage.
- screenshots: baselines/media/2026-10-01_wii_system_rice_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.20 | 0.995 | 64.2 | 27.9 | 14189 | 1572 | 14 | 13 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.21 | 0.995 | 70.2 | 27.5 | 15516 | 686 | 14 | 5 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.27 | 0.982 | 50.2 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Mario Party.v64 | vis | 900 | 15.20 | 0.987 | 71.8 | 57.9 | 7672 | 1420 | 25 | 7 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.17 | 0.991 | 80.6 | 45.5 | 8399 | 1258 | 17 | 4 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.17 | 0.991 | 75.9 | 46.5 | 7855 | 1485 | 18 | 1 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.21 | 0.986 | 36.2 | 54.4 | 10825 | 777 | 12 | 0 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.20 | 0.987 | 50.6 | 54.4 | 15862 | 842 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_system_rice_full2 <other id or run dir>`
