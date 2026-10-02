# 2026-10-02_hw_memo_dcbz_rejected_reference

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 84b3df6+cpu-opt
- purpose: Reference_for_the_rejected_memo/dcbz_candidate
- screenshots: baselines/media/2026-10-02_hw_memo_dcbz_rejected_reference/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 59.7 | 27.9 | 14190 | 1572 | 14 | 13 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.37 | 0.978 | 23.0 | 27.1 | 25080 | 1508 | 14 | 108 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.977 | 41.1 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.37 | 0.976 | 13.9 | 53.9 | 10823 | 777 | 12 | 6 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 38.8 | 54.5 | 15951 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.19 | 0.988 | 66.2 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 41.1 | 17.1 | 6595 | 866 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 68.3 | 26.2 | 6302 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.26 | 0.983 | 59.6 | 26.5 | 5085 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_memo_dcbz_rejected_reference <other id or run dir>`
