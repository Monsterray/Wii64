# 2026-10-01_wii_invalidation_control

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+invalidate-experiment-1.6.5
- purpose: Page-skip candidate/reference/candidate, equal reduced probes. Neutral 900-VI intros; opt-in, not a shipping gameplay-speed claim.
- screenshots: baselines/media/2026-10-01_wii_invalidation_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.987 | 52.0 | 25.4 | 5981 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 47.0 | 26.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.24 | 0.984 | 21.8 | 56.2 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_invalidation_control <other id or run dir>`
