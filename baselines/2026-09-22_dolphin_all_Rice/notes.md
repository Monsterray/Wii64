# 2026-09-22_dolphin_all_Rice

- date: 2026-09-22
- platform: dolphin
- plugin: Rice
- build: e9fdfa4+dirty
- purpose: all.txt (4 ROMs x 3600 VIs), stock settings, first chained baseline. OoT MQ and Majora's Mask hang at boot (CIC-6105): black screen, ~0 fps; OoT MQ runs 50 Hz VI timing against a 60 Hz header, so its speed column is not meaningful while hung.
- screenshots: baselines/media/2026-09-22_dolphin_all_Rice/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 3600 | 60.02 | 1.000 | 41.1 | 28.2 | 27303 | 1567 | 15 | 7 |
| 2 | Banjo-Kazooie.V64 | vis | 3600 | 60.06 | 0.999 | 58.4 | 21.6 | 22438 | 2351 | 32 | 3 |
| 3 | Zelda Ocarina of Time Master Quest.v64 | vis | 3600 | 71.41 | 0.840 | 51.2 | 0.2 | 139800 | 480 | 9 | 845 |
| 4 | Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | vis | 3600 | 72.00 | 1.000 | 83.6 | 0.2 | 144446 | 565 | 10 | 1 |

Compare: `python scripts/chain_compare.py 2026-09-22_dolphin_all_Rice <other id or run dir>`
