# 2026-10-01_wii_invalidation_full2

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+invalidate-experiment-1.6.5
- purpose: Page-skip candidate/reference/candidate, equal reduced probes. Neutral 900-VI intros; opt-in, not a shipping gameplay-speed claim.
- screenshots: baselines/media/2026-10-01_wii_invalidation_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.988 | 52.5 | 25.4 | 5977 | 932 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.989 | 47.5 | 27.6 | 5450 | 2084 | 31 | 2 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.25 | 0.984 | 21.7 | 54.3 | 10825 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_invalidation_full2 <other id or run dir>`
