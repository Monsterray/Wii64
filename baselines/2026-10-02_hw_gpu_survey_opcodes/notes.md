# 2026-10-02_hw_gpu_survey_opcodes

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 2590d8b+gpu-counters
- purpose: GP hardware counters, 9 3D scenes (opcodes)
- screenshots: baselines/media/2026-10-02_hw_gpu_survey_opcodes/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 59.7 | 27.9 | 14188 | 1572 | 14 | 13 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.37 | 0.978 | 23.0 | 27.0 | 25071 | 1504 | 14 | 105 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 41.1 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.38 | 0.975 | 13.8 | 53.8 | 10823 | 777 | 12 | 8 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 39.1 | 54.5 | 15783 | 850 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.25 | 0.984 | 65.9 | 52.3 | 5543 | 837 | 15 | 5 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.31 | 0.980 | 40.6 | 17.1 | 6648 | 869 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.26 | 0.983 | 67.5 | 26.2 | 6301 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.47 | 0.969 | 59.7 | 26.5 | 5085 | 1389 | 15 | 6 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_gpu_survey_opcodes <other id or run dir>`
