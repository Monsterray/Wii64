# 2026-09-30_wii_paging_matched_b

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: matched-paging-full
- purpose: Matched combined paging, candidate B; identical DOL to candidate A
- screenshots: baselines/media/2026-09-30_wii_paging_matched_b/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 18.17 | 0.990 | 72.6 | 46.7 | 7875 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 15.18 | 0.988 | 79.3 | 28.8 | 10321 | 1179 | 19 | 1 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.17 | 0.989 | 65.6 | 26.6 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_paging_matched_b <other id or run dir>`
