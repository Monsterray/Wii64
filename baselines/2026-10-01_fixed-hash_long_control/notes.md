# 2026-10-01_fixed-hash_long_control

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Matched fixed-length XXH32 A/B/A long; control; same subsystem probes; neutral scenes, not full gameplay
- screenshots: baselines/media/2026-10-01_fixed-hash_long_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.25 | 0.996 | 42.2 | 28.2 | 27303 | 1567 | 15 | 12 |
| 2 | Banjo-Kazooie.V64 | vis | 3600 | 60.24 | 0.996 | 50.5 | 21.5 | 22438 | 2350 | 32 | 8 |
| 3 | Pokemon Snap.rom | vis | 3600 | 61.49 | 0.976 | 6.8 | 57.9 | 37959 | 832 | 12 | 123 |

Compare: `python scripts/chain_compare.py 2026-10-01_fixed-hash_long_control <other id or run dir>`
