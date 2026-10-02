# 2026-10-02_hw_dispatch_table_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Dynarec dispatch target table (full1; control = func tree every dispatch)
- screenshots: baselines/media/2026-10-02_hw_dispatch_table_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 62.5 | 28.0 | 14190 | 1572 | 14 | 11 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.33 | 0.995 | 43.5 | 27.6 | 25069 | 1504 | 14 | 17 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.977 | 44.3 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.29 | 0.981 | 18.4 | 54.2 | 10825 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 42.1 | 54.5 | 15864 | 842 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 69.1 | 52.4 | 5544 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.24 | 0.984 | 43.7 | 17.0 | 6594 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.990 | 69.4 | 26.2 | 6300 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.986 | 61.0 | 26.5 | 5085 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_dispatch_table_full1 <other id or run dir>`
