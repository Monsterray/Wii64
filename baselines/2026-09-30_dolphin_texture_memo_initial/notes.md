# 2026-09-30_dolphin_texture_memo_initial

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: 1.6.4+880d374e
- purpose: Initial generation-invalidated hash memo correctness, 3 VI targets passed; not accepted as faster
- screenshots: baselines/media/2026-09-30_dolphin_texture_memo_initial/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.01 | 0.999 | 50.0 | 25.4 | 5981 | 928 | 14 | 6 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.07 | 0.995 | 48.5 | 26.6 | 5450 | 2084 | 31 | 2 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.14 | 0.991 | 16.8 | 54.2 | 10824 | 777 | 12 | 2 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_texture_memo_initial <other id or run dir>`
