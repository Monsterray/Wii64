# library-20261001-dolphin-13

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-13/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party.v64 | vis | 1800 | 30.15 | 0.995 | 40.9 | 111.4 | 16088 | 1594 | 25 | 14 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-13 <other id or run dir>`
