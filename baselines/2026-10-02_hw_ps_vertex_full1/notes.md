# 2026-10-02_hw_ps_vertex_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Paired-single vertex loads (full1; control = C conversions)
- screenshots: baselines/media/2026-10-02_hw_ps_vertex_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 60.4 | 28.0 | 14190 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.40 | 0.993 | 41.7 | 27.5 | 25080 | 1504 | 14 | 19 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 41.2 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.32 | 0.979 | 15.5 | 54.1 | 10823 | 777 | 12 | 1 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 39.6 | 54.5 | 16004 | 856 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 67.0 | 52.4 | 5542 | 837 | 15 | 3 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 42.2 | 17.1 | 6595 | 866 | 12 | 10 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.17 | 0.989 | 68.0 | 26.2 | 6300 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.49 | 0.968 | 59.8 | 26.3 | 5084 | 1389 | 15 | 5 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_ps_vertex_full1 <other id or run dir>`
