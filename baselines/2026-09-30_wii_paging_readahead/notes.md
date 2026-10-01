# 2026-09-30_wii_paging_readahead

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.3+dirty HBC_AGENT=1 PERF_SUBSYSTEM_PROBES VM_PAGE_READAHEAD=1
- purpose: 900-VI neutral paging scenes; read-ahead only candidate between references; zero cache hits because dirty writeback invalidates windows
- screenshots: baselines/media/2026-09-30_wii_paging_readahead/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 19.77 | 0.910 | 67.4 | 46.8 | 7906 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 16.94 | 0.885 | 71.0 | 27.7 | 10316 | 1179 | 19 | 7 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 16.77 | 0.894 | 59.1 | 28.3 | 5087 | 1389 | 15 | 5 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_readahead <other id or run dir>`
