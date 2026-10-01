# 2026-10-01_fixed-hash_short_full2

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: f115e06+gdp-reset+fixed-hash
- purpose: Matched fixed-length XXH32 A/B/A short; full2; same subsystem probes; neutral scenes, not full gameplay
- screenshots: baselines/media/2026-10-01_fixed-hash_short_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.18 | 0.988 | 52.2 | 25.3 | 5978 | 928 | 14 | 7 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 47.1 | 26.6 | 5450 | 2084 | 31 | 2 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.24 | 0.984 | 23.8 | 54.4 | 10822 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_fixed-hash_short_full2 <other id or run dir>`
