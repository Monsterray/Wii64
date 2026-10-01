# 2026-10-01_fixed-hash_long_full2

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Matched fixed-length XXH32 A/B/A long; full2; same subsystem probes; neutral scenes, not full gameplay
- screenshots: baselines/media/2026-10-01_fixed-hash_long_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.29 | 0.995 | 42.6 | 28.2 | 27311 | 1567 | 15 | 16 |
| 2 | Banjo-Kazooie.V64 | vis | 3600 | 60.25 | 0.996 | 50.5 | 21.6 | 22438 | 2351 | 32 | 9 |
| 3 | Pokemon Snap.rom | vis | 3600 | 60.90 | 0.985 | 9.0 | 58.0 | 37958 | 832 | 12 | 63 |

Compare: `python scripts/chain_compare.py 2026-10-01_fixed-hash_long_full2 <other id or run dir>`
