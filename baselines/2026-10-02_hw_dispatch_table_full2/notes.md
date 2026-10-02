# 2026-10-02_hw_dispatch_table_full2

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Dynarec dispatch target table (full2; control = func tree every dispatch)
- screenshots: baselines/media/2026-10-02_hw_dispatch_table_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 62.5 | 28.0 | 14189 | 1572 | 14 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.34 | 0.994 | 43.5 | 27.5 | 25078 | 1504 | 14 | 14 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.976 | 44.0 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.31 | 0.980 | 18.4 | 54.2 | 10822 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 41.6 | 54.5 | 15936 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 69.1 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.24 | 0.984 | 43.5 | 17.1 | 6647 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 69.5 | 26.2 | 6302 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.986 | 61.1 | 26.5 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_dispatch_table_full2 <other id or run dir>`
