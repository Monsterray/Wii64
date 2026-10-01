# library-20261001-dolphin-03

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-03/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 1800 | 30.00 | 1.000 | 64.9 | 27.6 | 10764 | 1040 | 13 | 0 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-03 <other id or run dir>`
