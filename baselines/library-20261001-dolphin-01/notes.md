# library-20261001-dolphin-01

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-01/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 007 - The World Is Not Enough (USA).z64 | vis | 1800 | 30.21 | 0.993 | 81.9 | 28.9 | 14884 | 1624 | 25 | 6 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-01 <other id or run dir>`
