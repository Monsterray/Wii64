# library-20261002-hardware-03

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: f971efa+wip
- purpose: Library check after CPU optimizations (Wii, 6 of 18)
- screenshots: baselines/media/library-20261002-hardware-03/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party.v64 | vis | 1800 | 30.23 | 0.992 | 51.6 | 111.8 | 16085 | 1594 | 25 | 7 |
| 2 | Pokemon Snap.rom | vis | 1800 | 30.21 | 0.993 | 21.9 | 57.3 | 20232 | 790 | 12 | 0 |
| 3 | Super Mario 64.v64 | vis | 2400 | 40.23 | 0.994 | 68.0 | 28.0 | 14188 | 1572 | 14 | 12 |
| 4 | Super Smash Bros. (U) [!].z64 | vis | 1800 | 30.25 | 0.992 | 49.8 | 55.4 | 24466 | 1331 | 14 | 0 |
| 5 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 1800 | 30.28 | 0.991 | 46.5 | 18.6 | 12689 | 877 | 12 | 13 |
| 6 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 151.22 | 0.992 | 45.2 | 28.7 | 161356 | 2454 | 12 | 66 |

Compare: `python scripts/chain_compare.py library-20261002-hardware-03 <other id or run dir>`
