# library-20261001-hardware-03

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-hardware-03/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Party.v64 | vis | 1800 | 30.27 | 0.991 | 46.7 | 111.7 | 16084 | 1599 | 25 | 11 |
| 2 | Pokemon Snap.rom | vis | 1800 | 30.25 | 0.992 | 14.3 | 57.2 | 20232 | 790 | 12 | 0 |
| 3 | Super Mario 64.v64 | vis | 2400 | 40.20 | 0.995 | 64.3 | 28.0 | 14188 | 1572 | 14 | 10 |
| 4 | Super Smash Bros. (U) [!].z64 | vis | 1800 | 30.20 | 0.994 | 47.6 | 56.4 | 24584 | 1336 | 14 | 0 |
| 5 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 1800 | 30.25 | 0.992 | 42.7 | 18.6 | 12706 | 897 | 12 | 8 |
| 6 | Zelda Ocarina of Time Master Quest.v64 | vis | 9000 | 152.05 | 0.987 | 40.0 | 28.6 | 161441 | 2454 | 12 | 133 |

Compare: `python scripts/chain_compare.py library-20261001-hardware-03 <other id or run dir>`
