# 2026-10-02_hw_gpu_survey_full2

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 2590d8b+gpu-counters
- purpose: GP hardware counters, 9 3D scenes (full2)
- screenshots: baselines/media/2026-10-02_hw_gpu_survey_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.994 | 59.8 | 27.9 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.37 | 0.978 | 23.0 | 27.0 | 25065 | 1508 | 14 | 105 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.37 | 0.976 | 41.1 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.36 | 0.977 | 14.6 | 54.0 | 10824 | 777 | 12 | 5 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 39.6 | 54.5 | 16006 | 854 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.16 | 0.989 | 66.5 | 52.5 | 5544 | 837 | 15 | 3 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.30 | 0.980 | 41.0 | 17.1 | 6647 | 866 | 12 | 10 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.26 | 0.983 | 67.6 | 26.1 | 6298 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.47 | 0.970 | 59.7 | 26.5 | 5087 | 1389 | 15 | 6 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_gpu_survey_full2 <other id or run dir>`
