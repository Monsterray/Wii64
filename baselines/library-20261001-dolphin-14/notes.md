# library-20261001-dolphin-14

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-14/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Pokemon Snap.rom | vis | 1800 | 31.05 | 0.966 | 8.0 | 55.4 | 20231 | 790 | 12 | 100 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-14 <other id or run dir>`
