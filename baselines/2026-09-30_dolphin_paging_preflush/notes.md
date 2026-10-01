# 2026-09-30_dolphin_paging_preflush

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: 1.6.3 candidate, HBC agent, VM preflush=1, read-ahead=1
- purpose: 900-VI neutral large-ROM scenes; load-time writeback plus 32KiB read-ahead
- screenshots: baselines/media/2026-09-30_dolphin_paging_preflush/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 17.99 | 1.000 | 68.9 | 46.6 | 7919 | 1286 | 18 | 1 |
| 2 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 15.17 | 0.989 | 78.2 | 27.7 | 10314 | 1179 | 19 | 3 |
| 3 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.01 | 0.999 | 62.3 | 28.2 | 5085 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_paging_preflush <other id or run dir>`
