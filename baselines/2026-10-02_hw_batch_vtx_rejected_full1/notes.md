# 2026-10-02_hw_batch_vtx_rejected_full1

- date: 2026-10-02
- platform: hardware
- plugin: glN64
- build: 8dba042
- purpose: Rejected: triangle batches across VTX (full1; control = flush at VTX)
- screenshots: baselines/media/2026-10-02_hw_batch_vtx_rejected_full1/ (not in git)

| # | rom | how | vis | wall s | speed | idle % | fps | exceptions | recompiles | tree | underruns |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Super Mario 64.v64 | vis | 2400 | 40.21 | 0.995 | 59.9 | 28.0 | 14189 | 1572 | 14 | 12 |
| 2 | Mario Kart 64.v64 | vis | 3600 | 60.40 | 0.993 | 41.6 | 27.5 | 25079 | 1504 | 14 | 20 |
| 3 | Banjo-Kazooie.V64 | vis | 900 | 15.36 | 0.976 | 40.7 | 26.6 | 5450 | 2084 | 31 | 2 |
| 4 | Pokemon Snap.rom | vis | 900 | 15.32 | 0.979 | 15.2 | 54.0 | 10823 | 777 | 12 | 2 |
| 5 | Super Smash Bros. (U) [!].z64 | vis | 900 | 15.18 | 0.988 | 35.6 | 54.5 | 15823 | 849 | 14 | 0 |
| 6 | GoldenEye 007 (USA).z64 | vis | 900 | 15.19 | 0.988 | 67.4 | 52.4 | 5544 | 837 | 15 | 3 |
| 7 | Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | vis | 900 | 15.24 | 0.984 | 41.9 | 17.1 | 6647 | 866 | 12 | 9 |
| 8 | Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | vis | 900 | 15.17 | 0.989 | 67.3 | 26.2 | 6299 | 947 | 13 | 0 |
| 9 | Donkey Kong 64 (USA).z64 | vis | 900 | 15.22 | 0.985 | 59.8 | 26.6 | 5086 | 1389 | 15 | 3 |

Compare: `python scripts/chain_compare.py 2026-10-02_hw_batch_vtx_rejected_full1 <other id or run dir>`
