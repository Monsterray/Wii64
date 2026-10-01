# 2026-09-30_wii_graphics_probes_full1

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Matched graphics-probe overhead, full/control/full, paging defaults on
- screenshots: baselines/media/2026-09-30_wii_graphics_probes_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.17 | 0.989 | 53.0 | 25.4 | 5981 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 48.1 | 26.6 | 5450 | 2083 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.22 | 0.985 | 24.4 | 54.2 | 10823 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_graphics_probes_full1 <other id or run dir>`
