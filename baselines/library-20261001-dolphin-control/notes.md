# library-20261001-dolphin-control

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: Matched Snap then Kart; PERF_PROF only; XFB RAM copies; no concurrent compilation
- screenshots: baselines/media/library-20261001-dolphin-control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Pokemon Snap.rom | vis | 1800 | 30.26 | 0.992 | 11.7 | 56.9 | 20232 | 790 | 12 | 14 |
| 2 | Mario Kart 64.v64 | vis | 5400 | 91.07 | 0.988 | 21.3 | 28.3 | 40079 | 1732 | 14 | 95 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-control <other id or run dir>`
