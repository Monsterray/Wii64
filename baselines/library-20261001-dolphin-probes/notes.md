# library-20261001-dolphin-probes

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: Matched Snap then Kart; full probes; XFB RAM copies; no concurrent compilation
- screenshots: baselines/media/library-20261001-dolphin-probes/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Pokemon Snap.rom | vis | 1800 | 31.05 | 0.966 | 8.0 | 55.4 | 20232 | 790 | 12 | 100 |
| 2 | Mario Kart 64.v64 | vis | 5400 | 91.56 | 0.983 | 18.4 | 28.0 | 40085 | 1732 | 14 | 139 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-probes <other id or run dir>`
