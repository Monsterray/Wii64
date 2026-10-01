# 2026-10-01_hw_subsystem_schema2_full1

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 1.6.8-schema2-full
- purpose: Schema 2 subsystem probe overhead; IRQ-atomic self timers; frozen full/control/full survey
- screenshots: baselines/media/2026-10-01_hw_subsystem_schema2_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.988 | 62.1 | 25.1 | 5593 | 1065 | 14 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.35 | 0.978 | 23.1 | 27.0 | 25063 | 1508 | 14 | 105 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.35 | 0.977 | 15.4 | 54.0 | 10824 | 777 | 12 | 4 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.23 | 0.985 | 60.8 | 28.2 | 5086 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-01_hw_subsystem_schema2_full1 <other id or run dir>`
