# 2026-10-01_wii_invalidation_full1

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 76a86b6+invalidate-experiment-1.6.5
- purpose: Page-skip candidate/reference/candidate, equal reduced probes. Neutral 900-VI intros; opt-in, not a shipping gameplay-speed claim.
- screenshots: baselines/media/2026-10-01_wii_invalidation_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.988 | 52.4 | 25.4 | 5978 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 47.4 | 26.6 | 5450 | 2083 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.25 | 0.984 | 21.7 | 54.1 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-10-01_wii_invalidation_full1 <other id or run dir>`
