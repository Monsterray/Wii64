# 2026-09-30_wii_graphics_probes_control

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Matched graphics-probe overhead, full/control/full, paging defaults on
- screenshots: baselines/media/2026-09-30_wii_graphics_probes_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.988 | 55.2 | 25.4 | 5979 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 50.2 | 27.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.21 | 0.986 | 26.2 | 54.4 | 10823 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_graphics_probes_control <other id or run dir>`
