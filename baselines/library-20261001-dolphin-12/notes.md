# library-20261001-dolphin-12

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-12/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (USA).z64 | vis | 1800 | 30.00 | 1.000 | 78.6 | 62.0 | 12763 | 1915 | 20 | 4 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-12 <other id or run dir>`
