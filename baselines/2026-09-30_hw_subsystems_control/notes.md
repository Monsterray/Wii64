# 2026-09-30_hw_subsystems_control

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: ea7802e+subsystem-probes-1.6.1
- purpose: subsystem_survey: same scenes/settings, new subsystem probes disabled
- screenshots: baselines/media/2026-09-30_hw_subsystems_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.16 | 0.990 | 55.2 | 25.3 | 5981 | 928 | 14 | 7 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 50.2 | 26.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.21 | 0.986 | 26.6 | 54.4 | 10824 | 777 | 12 | 0 |
| 4 | Mario Party 3 (E) (M4) [!].z64 | vis | 900 | 20.62 | 0.873 | 65.0 | 48.3 | 7916 | 1286 | 18 | 1 |
| 5 | 007 - The World Is Not Enough (USA).z64 | vis | 900 | 18.02 | 0.832 | 66.8 | 27.8 | 10319 | 1179 | 19 | 16 |
| 6 | Donkey Kong 64 (USA).z64 | vis | 900 | 17.70 | 0.847 | 56.0 | 28.2 | 5086 | 1389 | 15 | 10 |

Compare: `python scripts/chain_compare.py 2026-09-30_hw_subsystems_control <other id or run dir>`
