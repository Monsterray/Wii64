# 2026-09-26_dolphin_glN64_11rom

- date: 2026-09-26
- platform: dolphin
- plugin: glN64
- build: ae4ad95
- purpose: 11 non-Zelda ROMs, 3600 VIs each; periodic A replay for Mario Party titles
- screenshots: baselines/media/2026-09-26_dolphin_glN64_11rom/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.06 | 0.999 | 43.0 | 28.2 | 27294 | 1567 | 15 | 0 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.00 | 1.000 | 98.7 | 0.8 | 6345 | 395 | 14 | 0 |
| 3 | Banjo-Kazooie.V64 | vis | 3600 | 60.05 | 0.999 | 54.8 | 21.9 | 22438 | 2351 | 32 | 0 |
| 4 | Mario Party.v64 | vis | 3600 | 63.41 | 0.946 | 25.1 | 131.8 | 33517 | 1627 | 25 | 0 |
| 5 | Mario Party (USA).z64 | vis | 3600 | 63.39 | 0.947 | 25.1 | 131.9 | 33519 | 1627 | 25 | 0 |
| 6 | Mario Party 2 (E) (M5) [!].z64 | vis | 3600 | 74.97 | 0.960 | 58.6 | 82.5 | 30364 | 2102 | 20 | 0 |
| 7 | Mario Party 2 (USA).z64 | vis | 3600 | 62.98 | 0.953 | 49.4 | 107.9 | 31349 | 2026 | 26 | 0 |
| 8 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 74.84 | 0.962 | 69.3 | 69.3 | 25308 | 2347 | 20 | 0 |
| 9 | Mario Party 3 (USA).z64 | vis | 3600 | 62.92 | 0.954 | 64.0 | 91.3 | 27038 | 2200 | 20 | 0 |
| 10 | Pokemon Snap.rom | vis | 3600 | 62.64 | 0.958 | 6.0 | 56.7 | 37960 | 832 | 12 | 0 |
| 11 | Super Smash Bros. (U) [!].z64 | vis | 3600 | 61.30 | 0.979 | 41.9 | 51.7 | 48849 | 2125 | 14 | 0 |

Compare: `python scripts/chain_compare.py 2026-09-26_dolphin_glN64_11rom <other id or run dir>`
