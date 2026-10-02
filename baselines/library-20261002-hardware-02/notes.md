# library-20261002-hardware-02

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: f971efa+wip
- purpose: Library check after CPU optimizations (Wii, 6 of 18)
- screenshots: baselines/media/library-20261002-hardware-02/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Kart 64.v64 | vis | 5400 | 90.29 | 0.997 | 54.6 | 28.4 | 38713 | 1711 | 14 | 8 |
| 2 | Mario Party (USA).z64 | vis | 1800 | 30.21 | 0.993 | 51.6 | 111.8 | 16082 | 1594 | 25 | 7 |
| 3 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.21 | 0.994 | 77.6 | 64.0 | 15562 | 2001 | 20 | 5 |
| 4 | Mario Party 2 (USA).z64 | vis | 1800 | 30.24 | 0.992 | 68.1 | 94.1 | 16148 | 1954 | 26 | 8 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.19 | 0.995 | 80.1 | 51.1 | 12823 | 2162 | 20 | 1 |
| 6 | Mario Party 3 (USA).z64 | vis | 1800 | 30.22 | 0.993 | 78.7 | 62.0 | 12802 | 1915 | 20 | 5 |

Compare: `python scripts/chain_compare.py library-20261002-hardware-02 <other id or run dir>`
