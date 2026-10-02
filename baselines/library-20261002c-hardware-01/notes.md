# library-20261002c-hardware-01

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: Library chain 1 on the final code (short store path)
- screenshots: baselines/media/library-20261002c-hardware-01/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.19 | 0.994 | 84.3 | 28.9 | 14881 | 1624 | 25 | 1 |
| 2 | Banjo-Kazooie.V64 | vis | 1800 | 30.18 | 0.994 | 63.9 | 23.8 | 10510 | 2324 | 32 | 4 |
| 3 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 1800 | 30.22 | 0.993 | 71.8 | 27.6 | 10764 | 1040 | 13 | 0 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 1800 | 30.21 | 0.993 | 57.8 | 27.6 | 9607 | 1694 | 17 | 3 |
| 5 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.18 | 0.994 | 71.5 | 54.5 | 11839 | 1080 | 15 | 5 |
| 6 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.70 | 0.996 | 54.2 | 24.8 | 169962 | 2608 | 15 | 8 |

Compare: `python scripts/chain_compare.py library-20261002c-hardware-01 <other id or run dir>`
