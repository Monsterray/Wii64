# library-20261001-dolphin-15

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-15/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 62.9 | 28.0 | 14190 | 1572 | 14 | 11 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-15 <other id or run dir>`
