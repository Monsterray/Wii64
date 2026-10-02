# 2026-10-02_hw_gpu_survey_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 2590d8b+gpu-counters
- purpose: GP hardware counters, 9 3D scenes (full1)
- screenshots: baselines/media/2026-10-02_hw_gpu_survey_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 59.9 | 28.0 | 14189 | 1572 | 14 | 15 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.38 | 0.978 | 23.0 | 27.0 | 25070 | 1504 | 14 | 104 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.32 | 0.979 | 41.1 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.39 | 0.975 | 14.5 | 53.9 | 10823 | 777 | 12 | 6 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 40.2 | 54.5 | 16095 | 855 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.17 | 0.989 | 66.4 | 52.4 | 5544 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 41.4 | 17.1 | 6647 | 866 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 68.3 | 26.2 | 6299 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.20 | 0.987 | 59.9 | 26.6 | 5086 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_gpu_survey_full1 <other id or run dir>`
