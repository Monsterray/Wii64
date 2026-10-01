# 2026-10-01_wii_system_rice_full1

- date: 2026-10-01
- platform: hardware
- plugin: Rice
- build: 76a86b6+system-probes-1.6.5
- purpose: Eight-ROM title/replay survey, original per-call invalidation probes. All loads and HBC returns validated; not full gameplay coverage.
- screenshots: baselines/media/2026-10-01_wii_system_rice_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 64.2 | 27.9 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 2400 | 40.18 | 0.995 | 70.2 | 27.5 | 15517 | 686 | 14 | 4 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.27 | 0.983 | 50.2 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Mario Party.v64 | vis | 900 | 15.20 | 0.987 | 71.6 | 58.0 | 7671 | 1427 | 25 | 7 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.18 | 0.990 | 80.4 | 47.0 | 8396 | 1263 | 17 | 4 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.15 | 0.992 | 75.9 | 46.4 | 7854 | 1485 | 18 | 1 |
| 7 | Pokemon Snap.rom | vis | 900 | 15.21 | 0.986 | 36.3 | 54.4 | 10823 | 777 | 12 | 0 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.20 | 0.987 | 50.9 | 54.4 | 15795 | 847 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_system_rice_full1 <other id or run dir>`
