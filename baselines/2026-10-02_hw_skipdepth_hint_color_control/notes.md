# 2026-10-02_hw_skipdepth_hint_color_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: f971efa+wip
- purpose: Kart depth-switch skip, texture CRC hint, PS color stores (control; control = all off)
- screenshots: baselines/media/2026-10-02_hw_skipdepth_hint_color_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 62.6 | 27.9 | 14190 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.34 | 0.994 | 43.6 | 27.5 | 25135 | 1504 | 14 | 15 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 44.3 | 26.6 | 5450 | 2084 | 31 | 4 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.30 | 0.980 | 18.9 | 54.2 | 10823 | 777 | 12 | 0 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 41.8 | 54.5 | 15817 | 849 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 69.3 | 52.4 | 5543 | 837 | 15 | 5 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 43.9 | 17.1 | 6648 | 866 | 12 | 8 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 69.6 | 26.2 | 6302 | 947 | 13 | 1 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.986 | 61.1 | 26.5 | 5086 | 1389 | 15 | 4 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_skipdepth_hint_color_control <other id or run dir>`
