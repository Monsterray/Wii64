# 2026-09-30_wii_texture_memo_initial_full2

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Initial generation-invalidated hash memo, matched candidate/reference/candidate; rejected for lack of improvement
- screenshots: baselines/media/2026-09-30_wii_texture_memo_initial_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.987 | 52.9 | 25.4 | 5979 | 928 | 14 | 7 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 48.0 | 27.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.26 | 0.983 | 23.5 | 54.4 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_texture_memo_initial_full2 <other id or run dir>`
