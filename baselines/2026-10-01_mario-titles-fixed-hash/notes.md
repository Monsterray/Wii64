# 2026-10-01_mario-titles-fixed-hash

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Full RDP reset and fixed hash dispatch: MK64 map selection with 18-record replay; MP1 scene, MP2/3 intro checks, not full-game coverage
- screenshots: baselines/media/2026-10-01_mario-titles-fixed-hash/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 600 | 10.18 | 0.982 | 63.5 | 22.1 | 3755 | 927 | 14 | 7 |
| 2 | Mario Kart 64.v64 | vis | 5400 | 91.32 | 0.986 | 23.5 | 28.4 | 32280 | 767 | 14 | 99 |
| 3 | Mario Party.v64 | vis | 900 | 15.21 | 0.986 | 73.1 | 54.9 | 7668 | 1420 | 25 | 6 |
| 4 | Mario Party 2 (E) (M5) [!].z64 | vis | 900 | 18.19 | 0.989 | 80.1 | 47.0 | 8400 | 1258 | 17 | 5 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.16 | 0.991 | 75.8 | 47.8 | 7848 | 1485 | 18 | 1 |

Compare: `python scripts/chain_compare.py 2026-10-01_mario-titles-fixed-hash <other id or run dir>`
