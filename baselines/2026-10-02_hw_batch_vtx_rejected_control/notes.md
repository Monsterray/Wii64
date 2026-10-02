# 2026-10-02_hw_batch_vtx_rejected_control

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Rejected: triangle batches across VTX (control; control = flush at VTX)
- screenshots: baselines/media/2026-10-02_hw_batch_vtx_rejected_control/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.995 | 59.8 | 28.0 | 14188 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.38 | 0.994 | 41.8 | 27.5 | 25067 | 1504 | 14 | 18 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.35 | 0.977 | 40.8 | 26.6 | 5450 | 2083 | 31 | 4 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.34 | 0.978 | 15.3 | 54.1 | 10823 | 777 | 12 | 1 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.19 | 0.988 | 36.1 | 54.4 | 15833 | 848 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 66.9 | 52.4 | 5543 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 42.1 | 17.1 | 6648 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 67.3 | 26.2 | 6302 | 946 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.23 | 0.985 | 59.8 | 26.5 | 5087 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_batch_vtx_rejected_control <other id or run dir>`
