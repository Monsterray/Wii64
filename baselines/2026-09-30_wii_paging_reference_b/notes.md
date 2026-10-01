# 2026-09-30_wii_paging_reference_b

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.3+dirty HBC_AGENT=1 PERF_SUBSYSTEM_PROBES VM_PAGE_READAHEAD=0
- purpose: 900-VI neutral paging scenes; reference repeat after read-ahead; agent enabled; no ROM preflush
- screenshots: baselines/media/2026-09-30_wii_paging_reference_b/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.63 | 0.873 | 64.3 | 46.9 | 7905 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.08 | 0.830 | 66.3 | 27.8 | 10320 | 1179 | 19 | 16 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.71 | 0.847 | 55.3 | 28.2 | 5086 | 1389 | 15 | 10 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_reference_b <other id or run dir>`
