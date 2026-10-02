# 2026-10-02_hw_ps_vertex_full2

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Paired-single vertex loads (full2; control = C conversions)
- screenshots: baselines/media/2026-10-02_hw_ps_vertex_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 60.5 | 28.0 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.39 | 0.994 | 41.9 | 27.5 | 25065 | 1504 | 14 | 20 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 41.2 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.32 | 0.979 | 15.5 | 54.1 | 10823 | 777 | 12 | 1 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 39.2 | 54.5 | 15880 | 850 | 14 | 1 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 67.0 | 52.4 | 5542 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 42.0 | 17.1 | 6647 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 68.3 | 26.2 | 6302 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.985 | 59.9 | 26.5 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_ps_vertex_full2 <other id or run dir>`
