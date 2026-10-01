# 2026-09-30_wii_texture_snapshot_control

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Byte-snapshot texture hash, matched candidate/reference/candidate; opt-in due SM64/Banjo CPU regression
- screenshots: baselines/media/2026-09-30_wii_texture_snapshot_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.19 | 0.987 | 53.0 | 25.4 | 5980 | 928 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.16 | 0.989 | 48.1 | 27.6 | 5450 | 2084 | 31 | 2 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.23 | 0.985 | 24.5 | 54.4 | 10823 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_texture_snapshot_control <other id or run dir>`
