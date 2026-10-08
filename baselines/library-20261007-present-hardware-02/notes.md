# library-20261007-present-hardware-02

- date: 2026-10-08
- platform: hardware
- plugin: glN64
- build: f254d63+43+wip
- purpose: Library check of the MP3 presentation fix (Wii, part 2 of 3)
- screenshots: baselines/media/library-20261007-present-hardware-02/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Kart 64.v64 | vis | 5400 | 90.30 | 0.997 | 58.7 | 28.4 | 39898 | 1734 | 14 | 7 |
| 2 | Mario Party (USA).z64 | vis | 1800 | 30.19 | 0.994 | 54.7 | 111.8 | 16084 | 1594 | 25 | 6 |
| 3 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.20 | 0.995 | 79.2 | 64.0 | 15566 | 2001 | 20 | 5 |
| 4 | Mario Party 2 (USA).z64 | vis | 1800 | 30.24 | 0.992 | 70.3 | 94.1 | 16166 | 1954 | 26 | 8 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.19 | 0.995 | 81.3 | 51.1 | 12831 | 2162 | 20 | 1 |
| 6 | Mario Party 3 (USA).z64 | vis | 1800 | 30.20 | 0.993 | 79.8 | 61.9 | 12786 | 1915 | 20 | 6 |

Compare: `python scripts/chain_compare.py library-20261007-present-hardware-02 <other id or run dir>`
