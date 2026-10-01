# 2026-10-01_fixed-hash_long_full1

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Matched fixed-length XXH32 A/B/A long; full1; same subsystem probes; neutral scenes, not full gameplay
- screenshots: baselines/media/2026-10-01_fixed-hash_long_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.29 | 0.995 | 42.6 | 28.1 | 27282 | 1567 | 15 | 13 |
| 2 | Banjo-Kazooie.V64 | vis | 3600 | 60.26 | 0.996 | 50.5 | 21.7 | 22438 | 2351 | 32 | 8 |
| 3 | Pokemon Snap.rom | vis | 3600 | 60.91 | 0.985 | 9.1 | 57.9 | 37959 | 832 | 12 | 62 |

Compare: `python scripts/chain_compare.py 2026-10-01_fixed-hash_long_full1 <other id or run dir>`
