# 2026-09-30_hw_subsystems_full2

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: ea7802e+subsystem-probes-1.6.1
- purpose: subsystem_survey: same frozen full DOL, A/B/A repeat, 900 VIs per neutral scene
- screenshots: baselines/media/2026-09-30_hw_subsystems_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.17 | 0.989 | 53.3 | 25.4 | 5979 | 928 | 14 | 9 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 48.3 | 26.6 | 5450 | 2083 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.21 | 0.986 | 25.1 | 56.3 | 10823 | 777 | 12 | 0 |
| 4 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.66 | 0.871 | 64.3 | 46.9 | 7905 | 1286 | 18 | 1 |
| 5 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.04 | 0.831 | 66.4 | 28.9 | 10307 | 1179 | 19 | 16 |
| 6 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.74 | 0.846 | 55.3 | 26.6 | 5087 | 1389 | 15 | 9 |

Compare: `python scripts/chain_compare.py 2026-09-30_hw_subsystems_full2 <other id or run dir>`
