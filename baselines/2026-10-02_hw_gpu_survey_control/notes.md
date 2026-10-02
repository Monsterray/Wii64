# 2026-10-02_hw_gpu_survey_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 2590d8b+gpu-counters
- purpose: GP hardware counters, 9 3D scenes (control)
- screenshots: baselines/media/2026-10-02_hw_gpu_survey_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 65.8 | 28.0 | 14190 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.01 | 0.983 | 28.8 | 27.2 | 25078 | 1504 | 14 | 77 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.19 | 0.988 | 49.8 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.23 | 0.985 | 25.6 | 54.4 | 10823 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 46.9 | 54.5 | 15831 | 848 | 14 | 1 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 71.5 | 52.5 | 5543 | 837 | 15 | 5 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.22 | 0.986 | 47.9 | 17.1 | 6648 | 869 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.990 | 72.9 | 26.3 | 6302 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.18 | 0.988 | 65.4 | 26.5 | 5085 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_gpu_survey_control <other id or run dir>`
