# 2026-10-02_hw_batch_vtx_rejected_full2

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Rejected: triangle batches across VTX (full2; control = flush at VTX)
- screenshots: baselines/media/2026-10-02_hw_batch_vtx_rejected_full2/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.22 | 0.994 | 59.9 | 28.0 | 14189 | 1572 | 14 | 14 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.39 | 0.994 | 41.7 | 27.5 | 25081 | 1504 | 14 | 17 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.977 | 40.8 | 26.6 | 5450 | 2083 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.32 | 0.979 | 15.2 | 54.1 | 10824 | 777 | 12 | 2 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.17 | 0.989 | 36.5 | 54.4 | 15995 | 853 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.18 | 0.988 | 67.4 | 52.4 | 5542 | 837 | 15 | 4 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.23 | 0.985 | 41.9 | 17.1 | 6648 | 866 | 12 | 7 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.16 | 0.989 | 67.3 | 26.2 | 6304 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.23 | 0.985 | 59.8 | 26.5 | 5085 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_batch_vtx_rejected_full2 <other id or run dir>`
