# 2026-10-01_mk64-warm-reference

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06-matched-reference
- purpose: Warm-load regression reference: SM64 600 -> MK64 3600; MK64 stalled despite completed VIs; not a healthy performance baseline
- screenshots: baselines/media/2026-10-01_mk64-warm-reference/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 600 | 10.19 | 0.982 | 63.3 | 22.1 | 3754 | 927 | 14 | 8 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.20 | 0.997 | 98.5 | 1.1 | 6347 | 403 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_mk64-warm-reference <other id or run dir>`
