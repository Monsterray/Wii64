# 2026-10-02_hw_skipdepth_hint_color_full2

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: f971efa+wip
- purpose: Kart depth-switch skip, texture CRC hint, PS color stores (full2; control = all off)
- screenshots: baselines/media/2026-10-02_hw_skipdepth_hint_color_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.23 | 0.994 | 62.9 | 27.9 | 14189 | 1572 | 14 | 13 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.25 | 0.996 | 54.2 | 27.6 | 25087 | 1508 | 14 | 8 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.31 | 0.980 | 44.2 | 26.6 | 5450 | 2084 | 31 | 3 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.27 | 0.982 | 19.9 | 54.2 | 10824 | 777 | 12 | 1 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 42.2 | 54.5 | 15930 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 69.6 | 52.4 | 5545 | 837 | 15 | 3 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.24 | 0.985 | 44.6 | 17.1 | 6595 | 879 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.990 | 69.6 | 26.3 | 6302 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.21 | 0.986 | 61.0 | 26.5 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_skipdepth_hint_color_full2 <other id or run dir>`
