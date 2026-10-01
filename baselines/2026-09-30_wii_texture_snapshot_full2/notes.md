# 2026-09-30_wii_texture_snapshot_full2

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Byte-snapshot texture hash, matched candidate/reference/candidate; opt-in due SM64/Banjo CPU regression
- screenshots: baselines/media/2026-09-30_wii_texture_snapshot_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.20 | 0.987 | 52.5 | 25.4 | 5978 | 928 | 14 | 7 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.18 | 0.988 | 47.5 | 26.6 | 5450 | 2084 | 31 | 3 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.22 | 0.985 | 25.9 | 54.4 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_texture_snapshot_full2 <other id or run dir>`
