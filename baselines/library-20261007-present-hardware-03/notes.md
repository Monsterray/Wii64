# library-20261007-present-hardware-03

- date: 2026-10-08
- platform: hardware
- plugin: glN64
- build: f254d63+43+wip
- purpose: Library check of the MP3 presentation fix (Wii, part 3 of 3)
- screenshots: baselines/media/library-20261007-present-hardware-03/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party.v64 | vis | 1800 | 30.22 | 0.993 | 54.6 | 111.8 | 16082 | 1594 | 25 | 7 |
| 2 | Pokemon Snap.rom | vis | 1800 | 30.19 | 0.994 | 27.7 | 57.4 | 20231 | 790 | 12 | 0 |
| 3 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.994 | 70.6 | 28.0 | 14188 | 1572 | 14 | 12 |
| 4 | Super Smash Bros. (U) [!].z64 | vis | 1800 | 30.20 | 0.993 | 52.3 | 55.4 | 24584 | 1336 | 14 | 0 |
| 5 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 1800 | 30.29 | 0.991 | 49.6 | 18.6 | 12705 | 877 | 12 | 11 |
| 6 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 150.83 | 0.994 | 47.9 | 28.9 | 161402 | 2454 | 12 | 32 |

Compare: `python scripts/chain_compare.py library-20261007-present-hardware-03 <other id or run dir>`
