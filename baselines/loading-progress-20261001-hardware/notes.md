# loading-progress-20261001-hardware

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 192be2b+loading-ui-1.6.7
- purpose: Progress-enabled VM loads for both Zelda ROMs; full probes; completed and returned HBC
- screenshots: baselines/media/loading-progress-20261001-hardware/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Zelda Ocarina of Time Master Quest.v64 | vis | 900 | 15.60 | 0.961 | 30.3 | 18.0 | 16777 | 1677 | 11 | 5 |
| 2 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 900 | 18.41 | 0.978 | 54.1 | 19.9 | 33706 | 1761 | 13 | 4 |

Compare: `python scripts/chain_compare.py loading-progress-20261001-hardware <other id or run dir>`
