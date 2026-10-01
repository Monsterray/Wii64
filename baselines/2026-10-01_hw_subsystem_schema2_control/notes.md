# 2026-10-01_hw_subsystem_schema2_control

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 1.6.8-schema2-control
- purpose: Schema 2 subsystem probe overhead; IRQ-atomic self timers; frozen full/control/full survey
- screenshots: baselines/media/2026-10-01_hw_subsystem_schema2_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.18 | 0.988 | 68.3 | 25.0 | 5594 | 1065 | 14 | 9 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.97 | 0.984 | 29.2 | 27.5 | 25069 | 1504 | 14 | 74 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.26 | 0.983 | 27.7 | 54.9 | 10824 | 777 | 12 | 0 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.19 | 0.987 | 66.3 | 26.3 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-01_hw_subsystem_schema2_control <other id or run dir>`
