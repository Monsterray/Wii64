# 2026-09-30_wii_paging_preflush

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.3+dirty HBC_AGENT=1 PERF_SUBSYSTEM_PROBES VM_PAGE_READAHEAD=1 VM_ROM_PREFLUSH=1
- purpose: First combined preflush/read-ahead 900-VI neutral paging scenes; async agent startup; matched-source repeat survey pending; startup NAND-write tradeoff not included in gameplay metrics
- screenshots: baselines/media/2026-09-30_wii_paging_preflush/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.18 | 0.990 | 72.6 | 46.7 | 7885 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 15.18 | 0.988 | 79.4 | 28.8 | 10316 | 1179 | 19 | 1 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.18 | 0.988 | 65.5 | 26.6 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_preflush <other id or run dir>`
