# 2026-09-30_hw_subsystems_full1

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: ea7802e+subsystem-probes-1.6.1
- purpose: subsystem_survey: 900 VIs per neutral scene, full probes, A/B/A first
- screenshots: baselines/media/2026-09-30_hw_subsystems_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.987 | 53.2 | 25.4 | 5980 | 932 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 48.3 | 27.6 | 5450 | 2084 | 31 | 4 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.22 | 0.985 | 25.1 | 54.4 | 10824 | 777 | 12 | 0 |
| 4 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.62 | 0.873 | 64.3 | 46.9 | 7919 | 1286 | 18 | 1 |
| 5 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.04 | 0.832 | 66.4 | 27.8 | 10315 | 1179 | 19 | 17 |
| 6 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.79 | 0.843 | 55.4 | 27.9 | 5087 | 1389 | 15 | 10 |

Compare: `python scripts/chain_compare.py 2026-09-30_hw_subsystems_full1 <other id or run dir>`
