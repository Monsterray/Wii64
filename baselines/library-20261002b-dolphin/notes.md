# library-20261002b-dolphin

- date: 2026-10-02
- platform: dolphin
- plugin: glN64
- build: cdea307+wip
- purpose: Library check after the JR lookup, invalidation and audio changes (Dolphin, 9 local ROMs)
- screenshots: baselines/media/library-20261002b-dolphin/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Banjo-Kazooie.V64 | vis | 1800 | 30.14 | 0.995 | 65.4 | 23.8 | 10510 | 2325 | 32 | 3 |
| 2 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 9000 | 180.37 | 0.998 | 51.5 | 24.8 | 169956 | 2608 | 15 | 3 |
| 3 | Mario Kart 64.v64 | vis | 5400 | 90.03 | 1.000 | 52.9 | 28.5 | 39899 | 1734 | 14 | 8 |
| 4 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.03 | 0.999 | 77.3 | 64.0 | 15564 | 2001 | 20 | 5 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.01 | 1.000 | 80.6 | 51.1 | 12815 | 2162 | 20 | 1 |
| 6 | Mario Party.v64 | vis | 1800 | 30.07 | 0.998 | 48.3 | 111.8 | 16087 | 1594 | 25 | 7 |
| 7 | Pokemon Snap.rom | vis | 1800 | 30.07 | 0.998 | 18.0 | 57.5 | 20231 | 790 | 12 | 0 |
| 8 | Super Mario 64.v64 | vis | 2400 | 40.01 | 1.000 | 70.4 | 28.1 | 14189 | 1572 | 14 | 9 |
| 9 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 151.08 | 0.993 | 45.3 | 28.8 | 161308 | 2454 | 12 | 48 |

Compare: `python scripts/chain_compare.py library-20261002b-dolphin <other id or run dir>`
