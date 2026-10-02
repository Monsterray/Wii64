# library-20261002b-hardware-02

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: Library check after the JR lookup, invalidation and audio changes (Wii, part 2 of 3)
- screenshots: baselines/media/library-20261002b-hardware-02/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Kart 64.v64 | vis | 5400 | 90.30 | 0.997 | 56.7 | 28.4 | 39899 | 1734 | 14 | 9 |
| 2 | Mario Party (USA).z64 | vis | 1800 | 30.20 | 0.993 | 53.3 | 111.8 | 16084 | 1594 | 25 | 7 |
| 3 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.21 | 0.994 | 78.5 | 64.0 | 15564 | 2001 | 20 | 5 |
| 4 | Mario Party 2 (USA).z64 | vis | 1800 | 30.23 | 0.992 | 69.5 | 94.1 | 16164 | 1954 | 26 | 7 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.18 | 0.995 | 80.8 | 51.1 | 12841 | 2162 | 20 | 1 |
| 6 | Mario Party 3 (USA).z64 | vis | 1800 | 30.20 | 0.993 | 79.3 | 61.9 | 12790 | 1915 | 20 | 5 |

Compare: `python scripts/chain_compare.py library-20261002b-hardware-02 <other id or run dir>`
