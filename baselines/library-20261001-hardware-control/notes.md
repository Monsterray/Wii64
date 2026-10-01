# library-20261001-hardware-control

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: PERF_PROF-only Kart first; GoldenEye and TWINE follow; same audio modes; HBC return verified
- screenshots: baselines/media/library-20261001-hardware-control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Kart 64.v64 | vis | 5400 | 91.08 | 0.988 | 25.1 | 28.2 | 39900 | 1734 | 14 | 79 |
| 2 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.17 | 0.994 | 66.8 | 54.5 | 11844 | 1081 | 15 | 5 |
| 3 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.18 | 0.994 | 83.5 | 28.9 | 14883 | 1624 | 25 | 1 |

Compare: `python scripts/chain_compare.py library-20261001-hardware-control <other id or run dir>`
