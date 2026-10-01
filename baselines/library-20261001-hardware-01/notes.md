# library-20261001-hardware-01

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-hardware-01/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.21 | 0.993 | 82.7 | 28.9 | 14890 | 1624 | 25 | 1 |
| 2 | Banjo-Kazooie.V64 | vis | 1800 | 30.19 | 0.994 | 55.7 | 24.3 | 10510 | 2325 | 32 | 3 |
| 3 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 1800 | 30.34 | 0.989 | 67.6 | 27.5 | 10761 | 1040 | 13 | 0 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 1800 | 30.35 | 0.989 | 51.4 | 27.5 | 9605 | 1694 | 17 | 4 |
| 5 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.35 | 0.989 | 64.3 | 55.5 | 11850 | 1081 | 15 | 10 |
| 6 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 181.48 | 0.992 | 46.8 | 24.6 | 169952 | 2608 | 15 | 38 |

Compare: `python scripts/chain_compare.py library-20261001-hardware-01 <other id or run dir>`
