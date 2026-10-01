# library-20261001-dolphin-07

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-07/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Kart 64.v64 | vis | 5400 | 91.48 | 0.984 | 18.4 | 28.0 | 39900 | 1734 | 14 | 131 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-07 <other id or run dir>`
