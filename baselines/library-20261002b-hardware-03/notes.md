# library-20261002b-hardware-03

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: cdea307+wip
- purpose: Library check after the JR lookup, invalidation and audio changes (Wii, part 3 of 3)
- screenshots: baselines/media/library-20261002b-hardware-03/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party.v64 | vis | 1800 | 30.22 | 0.993 | 53.3 | 111.8 | 16086 | 1594 | 25 | 7 |
| 2 | Pokemon Snap.rom | vis | 1800 | 30.23 | 0.992 | 25.4 | 57.4 | 20232 | 790 | 12 | 0 |
| 3 | Super Mario 64.v64 | vis | 2400 | 40.23 | 0.994 | 69.7 | 27.9 | 14189 | 1572 | 14 | 12 |
| 4 | Super Smash Bros. (U) [!].z64 | vis | 1800 | 30.22 | 0.993 | 52.3 | 55.4 | 24566 | 1333 | 14 | 0 |
| 5 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 1800 | 30.29 | 0.990 | 48.7 | 18.6 | 12688 | 877 | 12 | 11 |
| 6 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 151.33 | 0.991 | 50.7 | 23.3 | 159776 | 2440 | 12 | 72 |

Compare: `python scripts/chain_compare.py library-20261002b-hardware-03 <other id or run dir>`
