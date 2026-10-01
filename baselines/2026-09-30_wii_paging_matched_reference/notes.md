# 2026-09-30_wii_paging_matched_reference

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: matched-paging-control
- purpose: Matched reference: preflush/read-ahead off; same source, probes and agent as candidates
- screenshots: baselines/media/2026-09-30_wii_paging_matched_reference/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.60 | 0.874 | 64.3 | 46.8 | 7891 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.08 | 0.829 | 66.3 | 27.8 | 10318 | 1179 | 19 | 16 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.81 | 0.842 | 55.4 | 27.9 | 5085 | 1389 | 15 | 12 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_matched_reference <other id or run dir>`
