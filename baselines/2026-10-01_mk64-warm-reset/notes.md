# 2026-10-01_mk64-warm-reset

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06-gdp-reset
- purpose: Matched full gDP ROM reset: sustained graphics/audio after SM64 600 -> MK64 3600; neutral final Nintendo logo, not gameplay proof
- screenshots: baselines/media/2026-10-01_mk64-warm-reset/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 600 | 10.18 | 0.983 | 63.4 | 23.5 | 3755 | 927 | 14 | 8 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.76 | 0.988 | 20.5 | 28.1 | 29052 | 1518 | 14 | 53 |

Compare: `python scripts/chain_compare.py 2026-10-01_mk64-warm-reset <other id or run dir>`
