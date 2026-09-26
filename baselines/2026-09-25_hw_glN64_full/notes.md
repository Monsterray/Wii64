# 2026-09-25_hw_glN64_full

- date: 2026-09-25
- platform: hardware
- plugin: glN64
- build: 8b5cf63+dirty
- purpose: hardware.txt, nine-entry Wii baseline
- screenshots: baselines/media/2026-09-25_hw_glN64_full/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.23 | 0.996 | 45.2 | 28.3 | 27287 | 1567 | 15 | 13 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.20 | 0.997 | 98.6 | 0.9 | 6348 | 413 | 14 | 1 |
| 3 | Banjo-Kazooie.V64 | vis | 3600 | 60.23 | 0.996 | 53.4 | 21.8 | 22438 | 2351 | 32 | 6 |
| 4 | Mario Party.v64 | vis | 3600 | 63.06 | 0.952 | 31.8 | 132.0 | 33522 | 1626 | 25 | 25 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 3600 | 74.72 | 0.964 | 79.3 | 49.1 | 24486 | 973 | 16 | 4 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 74.77 | 0.963 | 72.4 | 49.5 | 24023 | 1288 | 18 | 2 |
| 7 | Pokemon Snap.rom | vis | 3600 | 60.64 | 0.989 | 11.5 | 58.2 | 37960 | 832 | 12 | 33 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 3600 | 60.40 | 0.993 | 45.6 | 53.0 | 48742 | 2124 | 14 | 7 |
| 9 | Super Mario 64.v64 | vis | 7200 | 120.57 | 0.995 | 40.7 | 27.2 | 53477 | 1092 | 14 | 28 |

Compare: `python scripts/chain_compare.py 2026-09-25_hw_glN64_full <other id or run dir>`
