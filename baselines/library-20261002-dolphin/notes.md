# library-20261002-dolphin

- date: 2026-10-02
- platform: dolphin
- plugin: glN64
- build: f971efa+wip
- purpose: Library check after CPU optimizations (Dolphin, 9 local ROMs)
- screenshots: baselines/media/library-20261002-dolphin/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Banjo-Kazooie.V64 | vis | 1800 | 30.07 | 0.998 | 62.3 | 23.8 | 10510 | 2324 | 32 | 3 |
| 2 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.42 | 0.998 | 49.3 | 24.8 | 169962 | 2608 | 15 | 3 |
| 3 | Mario Kart 64.v64 | vis | 5400 | 90.02 | 1.000 | 50.0 | 28.5 | 38156 | 1715 | 14 | 6 |
| 4 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.03 | 0.999 | 75.9 | 64.0 | 15563 | 2001 | 20 | 5 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.01 | 1.000 | 79.3 | 51.1 | 12812 | 2162 | 20 | 1 |
| 6 | Mario Party.v64 | vis | 1800 | 30.10 | 0.997 | 46.1 | 111.7 | 16088 | 1594 | 25 | 11 |
| 7 | Pokemon Snap.rom | vis | 1800 | 30.14 | 0.995 | 13.5 | 57.4 | 20232 | 790 | 12 | 2 |
| 8 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 67.6 | 28.0 | 14190 | 1572 | 14 | 10 |
| 9 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 151.73 | 0.989 | 42.4 | 28.7 | 161364 | 2454 | 12 | 116 |

Compare: `python scripts/chain_compare.py library-20261002-dolphin <other id or run dir>`
