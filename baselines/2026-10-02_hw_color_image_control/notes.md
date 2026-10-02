# 2026-10-02_hw_color_image_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 84b3df6+cpu-opt
- purpose: Kart_SETCIMG_original_reference
- screenshots: baselines/media/2026-10-02_hw_color_image_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.17 | 0.989 | 62.0 | 25.1 | 5593 | 1065 | 14 | 10 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 61.37 | 0.978 | 23.0 | 27.0 | 25068 | 1504 | 14 | 106 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.39 | 0.975 | 13.8 | 53.8 | 10823 | 777 | 12 | 9 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.986 | 59.9 | 26.5 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_color_image_control <other id or run dir>`
