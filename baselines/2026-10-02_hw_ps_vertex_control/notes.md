# 2026-10-02_hw_ps_vertex_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Paired-single vertex loads (control; control = C conversions)
- screenshots: baselines/media/2026-10-02_hw_ps_vertex_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 59.6 | 28.0 | 14190 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.32 | 0.995 | 41.7 | 27.5 | 25099 | 1504 | 14 | 13 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.976 | 41.0 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.44 | 0.971 | 13.0 | 53.7 | 10824 | 777 | 12 | 11 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.16 | 0.989 | 39.2 | 54.5 | 15999 | 854 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.17 | 0.989 | 66.0 | 52.4 | 5543 | 837 | 15 | 3 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 41.0 | 17.1 | 6647 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.18 | 0.988 | 68.1 | 26.2 | 6303 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.986 | 59.8 | 26.5 | 5087 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_ps_vertex_control <other id or run dir>`
