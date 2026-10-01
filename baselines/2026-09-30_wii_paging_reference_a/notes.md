# 2026-09-30_wii_paging_reference_a

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.3+dirty HBC_AGENT=1 PERF_SUBSYSTEM_PROBES VM_PAGE_READAHEAD=0
- purpose: 900-VI neutral paging scenes; reference A of reference/read-ahead/reference; agent enabled; no ROM preflush
- screenshots: baselines/media/2026-09-30_wii_paging_reference_a/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.61 | 0.873 | 64.3 | 46.9 | 7900 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.09 | 0.829 | 66.3 | 27.8 | 10317 | 1179 | 19 | 17 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.72 | 0.846 | 55.2 | 26.4 | 5085 | 1389 | 15 | 11 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_reference_a <other id or run dir>`
