# 2026-10-02_hw_dispatch_table_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Dynarec dispatch target table (control; control = func tree every dispatch)
- screenshots: baselines/media/2026-10-02_hw_dispatch_table_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 60.4 | 28.0 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.38 | 0.994 | 41.9 | 27.5 | 25066 | 1504 | 14 | 19 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 41.2 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.31 | 0.979 | 15.5 | 54.1 | 10823 | 777 | 12 | 1 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 40.5 | 54.4 | 15996 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.19 | 0.988 | 66.9 | 52.4 | 5545 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 42.4 | 17.1 | 6594 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 68.2 | 26.2 | 6302 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.21 | 0.986 | 59.8 | 26.5 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_dispatch_table_control <other id or run dir>`
