# library-20261001-dolphin-18

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-18/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 153.96 | 0.974 | 36.4 | 28.3 | 161372 | 2455 | 12 | 304 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-18 <other id or run dir>`
