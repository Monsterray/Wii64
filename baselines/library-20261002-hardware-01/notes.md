# library-20261002-hardware-01

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: f971efa+wip
- purpose: Library check after CPU optimizations (Wii, 6 of 18)
- screenshots: baselines/media/library-20261002-hardware-01/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.19 | 0.994 | 83.6 | 28.9 | 14891 | 1624 | 25 | 1 |
| 2 | Banjo-Kazooie.V64 | vis | 1800 | 30.17 | 0.994 | 60.5 | 23.8 | 10510 | 2325 | 32 | 5 |
| 3 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 1800 | 30.22 | 0.993 | 69.7 | 27.6 | 10761 | 1040 | 13 | 0 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 1800 | 30.22 | 0.993 | 54.9 | 27.6 | 9607 | 1695 | 17 | 3 |
| 5 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.19 | 0.994 | 68.9 | 54.5 | 11849 | 1081 | 15 | 6 |
| 6 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.87 | 0.995 | 51.4 | 24.8 | 169959 | 2608 | 15 | 8 |

Compare: `python scripts/chain_compare.py library-20261002-hardware-01 <other id or run dir>`
