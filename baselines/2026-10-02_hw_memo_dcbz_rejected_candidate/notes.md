# 2026-10-02_hw_memo_dcbz_rejected_candidate

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 84b3df6+cpu-opt
- purpose: Rejected:_TMEM_stamp_memo_plus_dcbz/dcbt_conversion
- screenshots: baselines/media/2026-10-02_hw_memo_dcbz_rejected_candidate/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 59.0 | 27.9 | 14189 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.58 | 0.990 | 37.7 | 27.5 | 25069 | 1504 | 14 | 33 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.31 | 0.980 | 39.9 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.40 | 0.974 | 13.9 | 53.8 | 10824 | 777 | 12 | 10 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 37.9 | 54.5 | 15949 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 66.2 | 52.4 | 5544 | 837 | 15 | 3 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.24 | 0.984 | 38.7 | 17.1 | 6648 | 878 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.18 | 0.988 | 67.7 | 26.1 | 6300 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.25 | 0.984 | 59.5 | 26.6 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_memo_dcbz_rejected_candidate <other id or run dir>`
