# library-20261001-dolphin-05

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-05/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | GoldenEye 007 (USA).z64 | vis | 1800 | 30.11 | 0.996 | 60.9 | 54.2 | 11840 | 1080 | 15 | 7 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-05 <other id or run dir>`
