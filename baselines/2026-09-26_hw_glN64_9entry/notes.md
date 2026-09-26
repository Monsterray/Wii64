# 2026-09-26_hw_glN64_9entry

- date: 2026-09-26
- platform: hardware
- plugin: glN64
- build: ae4ad95
- purpose: hardware.txt, eight ROMs and final SM64 pad sweep, 3600/7200 VIs
- screenshots: baselines/media/2026-09-26_hw_glN64_9entry/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.22 | 0.996 | 45.2 | 28.1 | 27300 | 1567 | 15 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.61 | 0.990 | 23.1 | 28.3 | 29186 | 1518 | 14 | 37 |
| 3 | Banjo-Kazooie.V64 | vis | 3600 | 60.25 | 0.996 | 53.3 | 21.7 | 22437 | 2350 | 32 | 6 |
| 4 | Mario Party.v64 | vis | 3600 | 63.18 | 0.950 | 31.8 | 133.3 | 33520 | 1626 | 25 | 28 |
| 5 | Mario Party 2 (E) (M5) [!].z64 | vis | 3600 | 74.83 | 0.962 | 79.3 | 49.0 | 24486 | 973 | 16 | 0 |
| 6 | Mario Party 3 (E) (M4) [!].z64 | vis | 3600 | 74.91 | 0.961 | 72.2 | 49.1 | 24093 | 1288 | 18 | 2 |
| 7 | Pokemon Snap.rom | vis | 3600 | 60.66 | 0.989 | 11.7 | 58.2 | 37959 | 832 | 12 | 21 |
| 8 | Super Smash Bros. (U) [!].z64 | vis | 3600 | 60.45 | 0.993 | 45.9 | 52.8 | 48945 | 2124 | 14 | 5 |
| 9 | Super Mario 64.v64 | vis | 7200 | 120.66 | 0.995 | 40.6 | 27.3 | 53491 | 1091 | 14 | 27 |

Compare: `python scripts/chain_compare.py 2026-09-26_hw_glN64_9entry <other id or run dir>`
