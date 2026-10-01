# library-20261001-dolphin-16

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-16/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Smash Bros. (U) [!].z64 | vis | 1800 | 30.06 | 0.998 | 46.1 | 55.4 | 24418 | 1333 | 14 | 0 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-16 <other id or run dir>`
