# library-20261001-dolphin-17

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-17/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 1800 | 30.12 | 0.996 | 36.5 | 18.6 | 12706 | 877 | 12 | 10 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-17 <other id or run dir>`
