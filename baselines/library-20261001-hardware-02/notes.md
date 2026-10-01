# library-20261001-hardware-02

- date: 2026-10-01
- platform: hardware
- plugin: glN64
- build: 192be2b+version-1.6.7
- purpose: 18-ROM library smoke; full subsystem probes; before loading UI change; intro/replay only
- screenshots: baselines/media/library-20261001-hardware-02/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Mario Kart 64.v64 | vis | 5400 | 91.33 | 0.985 | 23.0 | 28.1 | 40081 | 1732 | 14 | 102 |
| 2 | Mario Party (USA).z64 | vis | 1800 | 30.39 | 0.987 | 46.6 | 114.5 | 16087 | 1594 | 25 | 8 |
| 3 | Mario Party 2 (E) (M5) [!].z64 | vis | 1800 | 36.35 | 0.990 | 75.8 | 64.8 | 15564 | 2001 | 20 | 11 |
| 4 | Mario Party 2 (USA).z64 | vis | 1800 | 30.42 | 0.986 | 65.1 | 95.6 | 16161 | 1954 | 26 | 10 |
| 5 | Mario Party 3 (E) (M4) [!].z64 | vis | 1800 | 36.39 | 0.989 | 78.6 | 51.1 | 12830 | 2162 | 20 | 6 |
| 6 | Mario Party 3 (USA).z64 | vis | 1800 | 30.32 | 0.989 | 77.5 | 62.1 | 12792 | 1915 | 20 | 9 |

Compare: `python scripts/chain_compare.py library-20261001-hardware-02 <other id or run dir>`
