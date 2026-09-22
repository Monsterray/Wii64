# 2026-09-22_dolphin_all_glN64

- date: 2026-09-22
- platform: dolphin
- plugin: glN64
- build: e9fdfa4+dirty
- purpose: all.txt (4 ROMs x 3600 VIs), stock settings, first chained baseline. OoT MQ and Majora's Mask hang at boot (CIC-6105): black screen, ~0 fps; OoT MQ runs 50 Hz VI timing against a 60 Hz header, so its speed column is not meaningful while hung.
- screenshots: baselines/media/2026-09-22_dolphin_all_glN64/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.00 | 1.000 | 41.1 | 28.2 | 27300 | 1567 | 15 | 8 |
| 2 | Banjo-Kazooie.V64 | vis | 3600 | 60.08 | 0.999 | 54.0 | 21.6 | 22438 | 2351 | 32 | 2 |
| 3 | Zelda Ocarina of Time Master Quest.v64 | vis | 3600 | 71.36 | 0.841 | 51.1 | 0.2 | 139806 | 480 | 9 | 853 |
| 4 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 3600 | 71.98 | 1.000 | 83.5 | 0.2 | 144448 | 565 | 10 | 1 |

Compare: `python scripts/chain_compare.py 2026-09-22_dolphin_all_glN64 <other id or run dir>`
