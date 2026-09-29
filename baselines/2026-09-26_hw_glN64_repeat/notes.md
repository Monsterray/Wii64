# 2026-09-26_hw_glN64_repeat

- date: 2026-09-26
- platform: hardware
- plugin: glN64
- build: 0a05807
- purpose: Repeat nine-entry Wii chain before audio work
- screenshots: baselines/media/2026-09-26_hw_glN64_repeat/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.21 | 0.996 | 44.9 | 28.2 | 27292 | 1567 | 15 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.57 | 0.991 | 22.9 | 28.3 | 29110 | 1518 | 14 | 38 |
| 3 | Banjo-Kazooie.V64 | vis | 3600 | 60.21 | 0.997 | 53.2 | 21.6 | 22438 | 2351 | 32 | 6 |
| 4 | Mario Party.v64 | vis | 3600 | 63.10 | 0.951 | 31.9 | 133.4 | 33521 | 1626 | 25 | 26 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 3600 | 74.69 | 0.964 | 79.4 | 49.2 | 24485 | 973 | 16 | 2 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 74.94 | 0.961 | 72.2 | 49.4 | 24079 | 1288 | 18 | 2 |
| 7 | Pokemon Snap.rom | vis | 3600 | 60.68 | 0.989 | 11.8 | 58.2 | 37959 | 832 | 12 | 23 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 3600 | 60.44 | 0.993 | 45.8 | 52.9 | 49042 | 2126 | 14 | 1 |
| 9 | Super Mario 64.v64 | vis | 7200 | 120.63 | 0.995 | 41.0 | 27.0 | 53448 | 1092 | 14 | 30 |

Compare: `python scripts/chain_compare.py 2026-09-26_hw_glN64_repeat <other id or run dir>`
