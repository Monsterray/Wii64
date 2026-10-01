# 2026-10-01_hw_subsystem_schema2_full2

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 1.6.8-schema2-full
- purpose: Schema 2 subsystem probe overhead; IRQ-atomic self timers; frozen full/control/full survey
- screenshots: baselines/media/2026-10-01_hw_subsystem_schema2_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.18 | 0.988 | 62.1 | 25.1 | 5594 | 1065 | 14 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.21 | 0.980 | 23.0 | 27.1 | 25067 | 1504 | 14 | 91 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.33 | 0.979 | 15.4 | 54.1 | 10825 | 777 | 12 | 2 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.986 | 60.7 | 28.2 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-01_hw_subsystem_schema2_full2 <other id or run dir>`
