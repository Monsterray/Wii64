# library-20261001-dolphin-09

- date: 2026-10-01
- platform: dolphin
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-dolphin-09/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.03 | 0.999 | 73.9 | 64.0 | 15563 | 2001 | 20 | 5 |

Compare: `python scripts/chain_compare.py library-20261001-dolphin-09 <other id or run dir>`
