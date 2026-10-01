# 2026-10-01_fixed-hash_short_control

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Matched fixed-length XXH32 A/B/A short; control; same subsystem probes; neutral scenes, not full gameplay
- screenshots: baselines/media/2026-10-01_fixed-hash_short_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.987 | 52.1 | 25.4 | 5978 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 47.2 | 27.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.27 | 0.982 | 21.5 | 54.3 | 10823 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_fixed-hash_short_control <other id or run dir>`
