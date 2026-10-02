# library-20261002b-hardware-01

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: Library check after the JR lookup, invalidation and audio changes (Wii, part 1 of 3)
- screenshots: baselines/media/library-20261002b-hardware-01/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.22 | 0.993 | 83.5 | 28.9 | 14887 | 1624 | 25 | 1 |
| 2 | Banjo-Kazooie.V64 | vis | 1800 | 30.18 | 0.994 | 62.4 | 23.8 | 10510 | 2325 | 32 | 5 |
| 3 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 1800 | 30.22 | 0.993 | 70.5 | 27.6 | 10764 | 1040 | 13 | 0 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 1800 | 30.21 | 0.993 | 55.8 | 27.6 | 9606 | 1695 | 17 | 3 |
| 5 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.19 | 0.994 | 70.6 | 54.5 | 11840 | 1080 | 15 | 5 |
| 6 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.82 | 0.995 | 52.7 | 24.8 | 169961 | 2608 | 15 | 6 |

Compare: `python scripts/chain_compare.py library-20261002b-hardware-01 <other id or run dir>`
