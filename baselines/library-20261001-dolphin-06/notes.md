# library-20261001-dolphin-06

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-06/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.64 | 0.996 | 44.1 | 24.8 | 169959 | 2608 | 15 | 3 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-06 <other id or run dir>`
