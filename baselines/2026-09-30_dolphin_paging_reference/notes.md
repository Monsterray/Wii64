# 2026-09-30_dolphin_paging_reference

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: 1.6.3 reference, HBC agent, VM read-ahead=0
- purpose: 900-VI neutral large-ROM scenes; exact NAND I/O reference
- screenshots: baselines/media/2026-09-30_dolphin_paging_reference/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.52 | 0.877 | 60.7 | 46.9 | 7915 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.15 | 0.826 | 64.9 | 28.9 | 10318 | 1180 | 19 | 18 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.76 | 0.844 | 52.3 | 26.4 | 5086 | 1389 | 15 | 13 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_paging_reference <other id or run dir>`
