# library-20261001-dolphin-04

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-04/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Donkey Kong 64 (USA).z64 | vis | 1800 | 30.29 | 0.990 | 51.2 | 28.3 | 9606 | 1694 | 17 | 5 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-04 <other id or run dir>`
