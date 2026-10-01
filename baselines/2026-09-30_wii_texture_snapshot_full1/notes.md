# 2026-09-30_wii_texture_snapshot_full1

- date: 2026-09-30
- platform: hardware
- plugin: glN64
- build: 1.6.4
- purpose: Byte-snapshot texture hash, matched candidate/reference/candidate; opt-in due SM64/Banjo CPU regression
- screenshots: baselines/media/2026-09-30_wii_texture_snapshot_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.20 | 0.987 | 52.5 | 25.3 | 5980 | 932 | 14 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 900 | 15.17 | 0.988 | 47.5 | 26.6 | 5450 | 2083 | 31 | 2 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.22 | 0.986 | 25.9 | 54.2 | 10824 | 777 | 12 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-30_wii_texture_snapshot_full1 <other id or run dir>`
