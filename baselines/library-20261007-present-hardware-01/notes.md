# library-20261007-present-hardware-01

- date: 2026-10-08
- platform: hardware
- plugin: glN64
- build: f254d63+43+wip
- purpose: Library check of the MP3 presentation fix (Wii, part 1 of 3)
- screenshots: baselines/media/library-20261007-present-hardware-01/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.19 | 0.994 | 84.2 | 28.9 | 14886 | 1624 | 25 | 1 |
| 2 | Banjo-Kazooie.V64 | vis | 1800 | 30.18 | 0.994 | 63.6 | 23.8 | 10510 | 2324 | 32 | 5 |
| 3 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 1800 | 30.22 | 0.993 | 71.6 | 27.6 | 10763 | 1040 | 13 | 0 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 1800 | 30.21 | 0.993 | 56.9 | 27.5 | 9604 | 1694 | 17 | 3 |
| 5 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.17 | 0.994 | 71.4 | 54.6 | 11839 | 1080 | 15 | 4 |
| 6 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.74 | 0.996 | 53.9 | 24.8 | 169967 | 2608 | 15 | 7 |

Compare: `python scripts/chain_compare.py library-20261007-present-hardware-01 <other id or run dir>`
