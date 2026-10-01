# 2026-09-30_dolphin_texture_snapshot_long

- date: 2026-09-30
- platform: dolphin
- plugin: glN64
- build: 1.6.4+83e7f18f
- purpose: Byte-snapshot candidate longer chain, all 5 VI targets passed with no latest-boot invalid accesses, LLE MMU muted
- screenshots: baselines/media/2026-09-30_dolphin_texture_snapshot_long/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 62.9 | 28.0 | 14186 | 1572 | 14 | 10 |
| 2 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 72.02 | 1.000 | 71.3 | 69.4 | 25340 | 2347 | 20 | 1 |
| 3 | 007 - The World Is Not Enough (USA).z64 | vis | 3600 | 60.17 | 0.997 | 73.9 | 28.9 | 38055 | 2756 | 25 | 8 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 3600 | 60.01 | 1.000 | 46.7 | 29.2 | 18338 | 1780 | 17 | 4 |
| 5 | Super Mario 64.v64 | vis | 900 | 15.01 | 0.999 | 49.7 | 25.5 | 5979 | 928 | 14 | 5 |

Compare: `python scripts/chain_compare.py 2026-09-30_dolphin_texture_snapshot_long <other id or run dir>`
