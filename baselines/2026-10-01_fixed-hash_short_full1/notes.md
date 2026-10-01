# 2026-10-01_fixed-hash_short_full1

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Matched fixed-length XXH32 A/B/A short; full1; same subsystem probes; neutral scenes, not full gameplay
- screenshots: baselines/media/2026-10-01_fixed-hash_short_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.988 | 52.4 | 25.4 | 5981 | 928 | 14 | 9 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 47.1 | 26.6 | 5450 | 2083 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.23 | 0.985 | 23.8 | 56.3 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_fixed-hash_short_full1 <other id or run dir>`
