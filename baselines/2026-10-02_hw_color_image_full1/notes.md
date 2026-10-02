# 2026-10-02_hw_color_image_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 84b3df6+cpu-opt
- purpose: Kart_SETCIMG_fast_conversion_candidate
- screenshots: baselines/media/2026-10-02_hw_color_image_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 900 | 15.18 | 0.988 | 61.9 | 25.0 | 5593 | 1065 | 14 | 11 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.40 | 0.993 | 41.8 | 27.5 | 25082 | 1504 | 14 | 19 |
| 3 | Pokemon Snap.rom | vis | 900 | 15.42 | 0.973 | 13.0 | 53.7 | 10823 | 777 | 12 | 13 |
| 4 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.21 | 0.986 | 59.9 | 26.5 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_color_image_full1 <other id or run dir>`
