# 2026-10-02_hw_skipdepth_hint_color_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: f971efa+wip
- purpose: Kart depth-switch skip, texture CRC hint, PS color stores (full1; control = all off)
- screenshots: baselines/media/2026-10-02_hw_skipdepth_hint_color_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 62.9 | 28.0 | 14189 | 1572 | 14 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.26 | 0.996 | 54.2 | 27.6 | 25083 | 1504 | 14 | 9 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.31 | 0.979 | 44.2 | 26.6 | 5450 | 2083 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.28 | 0.981 | 19.9 | 54.2 | 10823 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 42.2 | 54.5 | 15813 | 848 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.17 | 0.989 | 69.6 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.24 | 0.984 | 44.3 | 17.1 | 6648 | 866 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 69.6 | 26.3 | 6302 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.21 | 0.986 | 61.0 | 26.5 | 5082 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_skipdepth_hint_color_full1 <other id or run dir>`
