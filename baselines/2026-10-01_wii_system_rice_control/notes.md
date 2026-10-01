# 2026-10-01_wii_system_rice_control

- date: 2026-10-01
- platform: hardware
- plugin: Rice
- build: 76a86b6+system-probes-1.6.5
- purpose: Eight-ROM title/replay survey, original per-call invalidation probes. All loads and HBC returns validated; not full gameplay coverage.
- screenshots: baselines/media/2026-10-01_wii_system_rice_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.20 | 0.995 | 67.3 | 27.9 | 14189 | 1572 | 14 | 11 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.20 | 0.995 | 72.6 | 27.6 | 15513 | 686 | 14 | 6 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 54.8 | 26.5 | 5450 | 2084 | 31 | 3 |
| 4 | Mario Party.v64 | vis | 900 | 15.20 | 0.987 | 74.8 | 54.9 | 7670 | 1420 | 25 | 7 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.18 | 0.990 | 82.3 | 47.0 | 8400 | 1258 | 17 | 4 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.17 | 0.990 | 77.7 | 47.8 | 7845 | 1485 | 18 | 1 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.21 | 0.986 | 41.8 | 54.4 | 10824 | 777 | 12 | 0 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.19 | 0.988 | 55.1 | 54.4 | 15797 | 847 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_system_rice_control <other id or run dir>`
