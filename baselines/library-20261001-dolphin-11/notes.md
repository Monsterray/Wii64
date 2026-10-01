# library-20261001-dolphin-11

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-11/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.01 | 1.000 | 77.9 | 51.1 | 12842 | 2162 | 20 | 1 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-11 <other id or run dir>`
