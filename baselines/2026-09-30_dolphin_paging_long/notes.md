# 2026-09-30_dolphin_paging_long

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: 1.6.4+d4066420
- purpose: Longer paging correctness, LLE MMU host muted, all VI targets passed
- screenshots: baselines/media/2026-09-30_dolphin_paging_long/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 63.5 | 28.0 | 14188 | 1572 | 14 | 10 |
| 2 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 72.02 | 1.000 | 72.0 | 69.6 | 25271 | 2347 | 20 | 1 |
| 3 | 007 - The World Is Not Enough (USA).z64 | vis | 3600 | 60.18 | 0.997 | 74.6 | 29.2 | 38056 | 2756 | 25 | 9 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 3600 | 60.02 | 1.000 | 47.5 | 29.0 | 18336 | 1781 | 17 | 4 |
| 5 | Super Mario 64.v64 | vis | 900 | 15.01 | 0.999 | 50.2 | 25.5 | 5980 | 928 | 14 | 5 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_paging_long <other id or run dir>`
