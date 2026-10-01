# 2026-09-30_wii_texture_memo_initial_control

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Initial generation-invalidated hash memo, matched candidate/reference/candidate; rejected for lack of improvement
- screenshots: baselines/media/2026-09-30_wii_texture_memo_initial_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.987 | 52.9 | 25.4 | 5978 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 48.1 | 26.6 | 5450 | 2084 | 31 | 4 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.22 | 0.986 | 24.6 | 54.2 | 10823 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_texture_memo_initial_control <other id or run dir>`
